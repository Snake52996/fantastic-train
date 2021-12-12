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
    template<std::integral T>
    void writeInteger(std::ostream& output, T value){
        //Logger::trace("write ", value, '\n');
        // require little endian to work
        static_assert(std::endian::native == std::endian::little);
        static constexpr char MaxBytes = 8;
        static constexpr char CharMin = std::numeric_limits<char>::min();
        static constexpr char CharMax = std::numeric_limits<char>::max();
        char helper_buffer{0};
        bool negative{value < 0};
        // determain bytes required by the value
        if(std::cmp_less_equal(value, CharMax - MaxBytes) && std::cmp_greater_equal(value, CharMin + MaxBytes)){
            // very small value, save directly
            helper_buffer = static_cast<char>(value);
            if(helper_buffer != 0) helper_buffer = negative ? helper_buffer - MaxBytes : helper_buffer + MaxBytes;
            output.write(&helper_buffer, sizeof(char));
            return;
        }
        unsigned char bytes_required{0};
        if(negative) value = ~value;
        for(T temp_value = value; temp_value > 0; temp_value >>= CHAR_BIT) ++bytes_required;
        helper_buffer = bytes_required;
        if(negative) helper_buffer = -helper_buffer;
        output.write(&helper_buffer, sizeof(char));
        output.write(reinterpret_cast<char*>(&value), bytes_required);
    }
}
Saver::Saver(
    SimpleMessageQueue<SaveCommand>& command_queue, std::counting_semaphore<>& allowed_nodes
): command_queue_(command_queue), allowed_nodes_(allowed_nodes), file_("MCTs.new", std::ios::binary){
    this->file_.write(reinterpret_cast<const char*>(&MagicNumber), sizeof(std::size_t));
}
void Saver::waitForWorkers(MCTreeNode* target)const{
    Logger::debug("wait for workers on node ", target, '\n');
    std::size_t query_count{0};
    //Logger::debug("counter says ", target->workers_within_, '\n');
    while(*target->workers_within_ > 0){
        //Logger::debug("counter says ", target->workers_within_, '\n');
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
    Logger::debug("done waiting for workers on node ", target, '\n');
}
void Saver::saveOneUnlocked(MCTreeNode* target){
    // we believe there will be no workers on this node any more,
    //  failure of this assentation may cause all kinds of errors, including core dumps
    // structure of each node:
    //  size_t -- encoded_action_
    //  size_t -- number of children
    //   if number of children is 0:
    //    size_t -- number of total simulations
    //    for each player:
    //     size_t -- number of succeed simulations
    //    end for
    //   else
    //    for each child:
    //     structured value of the child node
    //    end for
    //   end if
    //Logger::trace("saveOneUnlocked ", target, '\n');
    writeInteger(this->file_, target->encoded_action_);
    writeInteger(this->file_, target->children_.size());
    if(target->children_.size() == 0){
        writeInteger(this->file_, target->totalVisit());
        for(std::size_t i = 0; i < target->validSucceedVisitNumber(); ++i){
            writeInteger(this->file_, target->succeedVisit(i));
        }
    }
}
void Saver::saveAllUnlocked(MCTreeNode* target){
    //Logger::trace("saveAllUnlocked ", target, '\n');
    // we believe there will be no workers on this sub tree any more,
    //  failure of this assentation may cause all kinds of errors, including core dumps
    this->saveOneUnlocked(target);
    if(target->children_.size() != 0){
        for(auto& [action, child]: target->children_) this->saveAllUnlocked(&child);
        // clear children map to free memory
        //Logger::trace("clearing children of ", target, '\n');
        std::size_t child_count{target->children_.size()};
        target->children_.clear();
        this->allowed_nodes_.release(child_count);
        //Logger::trace("done clearing children of ", target, '\n');
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
    //Logger::trace("saveChangeRoot from ", from, " to ", to, '\n');
    this->saveOneUnlocked(from);
    while(from->children_.size() > 1){
        auto iter = from->children_.begin();
        while(iter != from->children_.end()){
            Logger::debug("counter for node ", &iter->second, " reports ", *iter->second.workers_within_, '\n');
            if((&iter->second != to) && (*iter->second.workers_within_ == 0)){
                this->saveAll(&iter->second);
                //Logger::trace("erase ", &iter->second, '\n');
                from->children_.erase(iter++);
                this->allowed_nodes_.release();
            }else{
                ++iter;
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
                //Logger::trace("received exit command\n");
                return;
            case SaveCommand::Command::SaveAll:
                //Logger::trace("received save all command\n");
                this->saveAll(command.from);
                //Logger::trace("save all done\n");
                break;
            case SaveCommand::Command::ChangeRoot:
                //Logger::trace("received change root command\n");
                this->saveChangeRoot(command.from, command.to);
                //Logger::trace("change root done\n");
                break;
            case SaveCommand::Command::SavePlayerNumber:
                writeInteger(this->file_, command.number);
                break;
            default:
                break;
        }
    }
}