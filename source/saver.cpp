#include<includes/saver.hpp>
#include<includes/Logger.hpp>
#include<string>
#include<thread>
#include<chrono>
#include<bit>
#include<concepts>
#include<limits>
using namespace std::chrono_literals;
namespace{
    template<std::unsigned_integral T>
    void writeUnsignedInteger(std::ostream& output, T value){
        // require little endian to work
        static_assert(std::endian::native == std::endian::little);
        static constexpr T lowest_byte_mask = (static_cast<T>(1) << CHAR_BIT) - 1;
        unsigned char helper_buffer{0};
        // determain bytes required by the value
        if(value <= std::numeric_limits<unsigned char>::max() - 7){
            // very small value, save directly
            helper_buffer = static_cast<unsigned char>(value);
            if(helper_buffer != 0) helper_buffer += 7;
            output.write(reinterpret_cast<char*>(&helper_buffer), sizeof(unsigned char));
            return;
        }
        unsigned char bytes_required{0};
        for(T temp_value = value; temp_value > 0; temp_value >>= CHAR_BIT) ++bytes_required;
        helper_buffer = bytes_required - 1;
        output.write(reinterpret_cast<char*>(&helper_buffer), sizeof(unsigned char));
        for(unsigned char i = 0; i < bytes_required; ++i){
            helper_buffer = static_cast<unsigned char>((value >> (i * CHAR_BIT)) & lowest_byte_mask);
            output.write(reinterpret_cast<char*>(&helper_buffer), sizeof(unsigned char));
        }
    }
}
Saver::Saver(SimpleMessageQueue<SaveCommand>& command_queue): command_queue_(command_queue), file_("MCTs", std::ios::binary){
    this->file_.write(reinterpret_cast<const char*>(&MagicNumber), sizeof(std::size_t));
}
void Saver::waitForWorkers(MCTreeNode* target)const{
    Logger::debug("wait for workers on node ", target, '\n');
    std::size_t query_count{0};
    Logger::debug("counter says ", target->workers_within_, '\n');
    while(target->workers_within_ > 0){
        Logger::debug("counter says ", target->workers_within_, '\n');
        if(query_count == 5) Logger::debug("wait for workers blocked on node ", target, " for 125ms\n");
        else if(query_count == 10) Logger::info("wait for workers blocked on node ", target, " for 250ms\n");
        else if(query_count == 20) Logger::warning("wait for workers blocked on node ", target, " for 500ms\n");
        else if(query_count == 50){
            Logger::error("wait for workers blocked on node ", target, " for 1250ms, stop waiting and force save\n");
            Logger::error("  this may cause all kinds of errors including core dumps\n");
            break;
        }
        ++query_count;
        std::this_thread::sleep_for(25ms);
    }
}
void Saver::saveOneUnlocked(MCTreeNode* target){
    // we believe there will be no workers on this node any more,
    //  failure of this assentation may cause all kinds of errors, including core dumps
    // structure of each node:
    //  Unsigned Integer -- encoded_action_
    //  Unsigned Integer -- number of children
    //   if number of children is 0:
    //    Unsigned Integer -- number of total simulations
    //    for each player:
    //     Unsigned Integer -- number of succeed simulations
    //    end for
    //   else
    //    for each child:
    //     structured value of the child node
    //    end for
    //   end if
    Logger::trace("saving ", target, " with saveOneUnlocked\n");
    writeUnsignedInteger(this->file_, target->encoded_action_);
    writeUnsignedInteger(this->file_, target->children_.size());
    if(target->children_.size() == 0){
        writeUnsignedInteger(this->file_, target->totalVisit());
        for(std::size_t i = 0; i < target->validSucceedVisitNumber(); ++i){
            writeUnsignedInteger(this->file_, target->succeedVisit(i));
        }
    }
}
void Saver::saveAllUnlocked(MCTreeNode* target){
    Logger::trace("saving ", target, " with saveAllUnlocked\n");
    // we believe there will be no workers on this sub tree any more,
    //  failure of this assentation may cause all kinds of errors, including core dumps
    this->saveOneUnlocked(target);
    if(target->children_.size() != 0){
        for(auto& [action, child]: target->children_) this->saveAllUnlocked(&child);
        // clear children map to free memory
        Logger::trace("clearing children of ", target, '\n');
        target->children_.clear();
        Logger::trace("done clearing children of ", target, '\n');
    }
}
void Saver::saveAll(MCTreeNode* target){
    // wait till works exited from target sub tree
    this->waitForWorkers(target);
    // note that we assume that once this method is invoked, there shall be no more workers
    //  entering the target node and its sub tree, which is accomplished by the guide direction
    this->saveAllUnlocked(target);
}
void Saver::saveChangeRoot(MCTreeNode* from, MCTreeNode* to){
    Logger::trace("change root from ", from, " to ", to, '\n');
    this->saveOneUnlocked(from);
    while(from->children_.size() > 1){
        auto iter = from->children_.begin();
        while(iter != from->children_.end()){
            if((&iter->second != to) && (iter->second.workers_within_ == 0)){
                this->saveAll(&iter->second);
                Logger::trace("erase ", &iter->second, '\n');
                from->children_.erase(iter++);
            }
        }
        std::this_thread::sleep_for(20ms);
    }
}
void Saver::run(){
    while(true){
        auto command{this->command_queue_.get()};
        switch(command.command){
            case SaveCommand::Command::Exit:
                Logger::trace("received exit command\n");
                return;
            case SaveCommand::Command::SaveAll:
                Logger::trace("received save all command\n");
                this->saveAll(command.from);
                Logger::trace("save all done\n");
                break;
            case SaveCommand::Command::ChangeRoot:
                Logger::trace("received change root command\n");
                this->saveChangeRoot(command.from, command.to);
                Logger::trace("change root done\n");
                break;
            default:
                break;
        }
    }
}