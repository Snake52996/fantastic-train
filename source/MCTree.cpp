#include<includes/MCTree.hpp>
#include<includes/Logger.hpp>
#include<cmath>
#include<random>
#include<cstdlib>
#include<string>
#include<thread>
#include<mutex>
namespace{
    // this helper class helps to maintain the counter of workers within a specific sub tree
    //  since the method __step has multiple exit point, update the counter for each of them
    //  can be hard, especially during continuous updates.
    // by appling a scoped helper, the counter is automatically increased once the class is
    //  instantiated, and decreased when it runs out of scope.
    struct CounterHelper{
        std::atomic_size_t& counter_;
        CounterHelper(std::atomic_size_t& counter): counter_(counter){++counter_;}
        ~CounterHelper(){--counter_;}
    };
    // decrease only helper class which will only decrease the counter when running out of scope
    struct DecreaseOnlyHelper{
        std::atomic_size_t& counter_;
        DecreaseOnlyHelper(std::atomic_size_t& counter): counter_(counter){}
        ~DecreaseOnlyHelper(){--counter_;}
    };
    struct DebugCounterHelper{
        MCTreeNode* target_;
        DebugCounterHelper(MCTreeNode* target): target_(target){
            ++(*target_->workers_within_);
            Logger::debug("increased counter for node ", target_, '\n');
        }
        ~DebugCounterHelper(){
            --(*target_->workers_within_);
            Logger::debug("decreased counter for node ", target_, '\n');
        }
    };
}
MCTree::MCTree(
    const Game& game, SimpleMessageQueue<SaveCommand>& command_queue
): root_(game.setting_.player_count_, 0), current_root_(&root_), command_queue_(command_queue){
    --(*this->root_.workers_within_);
}
MCTree::MCTree(
    MCTreeNode&& root, SimpleMessageQueue<SaveCommand>& command_queue
): root_(std::move(root)), current_root_(&root_), command_queue_(command_queue){}
MCTree::~MCTree(){}
std::pair<bool, std::size_t> MCTree::__step(MCTreeNode* target, const Game& game, Game::State& state, std::size_t player_id)const{
    //Logger::debug("workers_within for ", target, " located at ", target->workers_within_, '\n');
    //CounterHelper helper(*(target->workers_within_));
    DebugCounterHelper helper(target);
    // reached a terminal node, return the result directly
    if(state.ended()) return std::make_pair(true, static_cast<std::size_t>(state.winner_));
    // the real action in target status has been decided, we must follow it
    if(target->guide_direction_ != nullptr){
        state = game.step(state, game.decodeAction(target->guide_direction_->encoded_action_));
        return this->__step(target->guide_direction_, game, state, player_id);
    }
    // stepping into the future
    // tools for generating random values
    std::random_device rd;
    std::mt19937_64 engine(rd());
    std::uniform_real_distribution distribution(0.0, 1.0);
    // save result for simplify
    std::pair<bool, std::size_t> result;
    // next node to explore
    MCTreeNode* next{nullptr};
    // we must expend, otherwise errors may be yield
    bool expand_only{target->children_.size() == 0};
    // we will try to expand, but may take a step back
    bool expand{expand_only || distribution(engine) <= MCTree::exploration_rate};
    if((!expand) ||  (next = target->expend(game, state)) == nullptr){
        // we will not or failed to expand
        // if we must expand indeed, a failure has occurred, give this episode up
        if(expand_only) return std::make_pair(false, 0);
        double best_reward{0.0};
        for(auto& item: target->children_){
            auto temp_reward{item.second.UCT(player_id, this->current_root_->totalVisit(), MCTree::lambda)};
            if(next == nullptr || temp_reward > best_reward){
                next = &(item.second);
                best_reward = temp_reward;
            }
        }
        state = game.step(state, game.decodeAction(next->encoded_action_));
        result = this->__step(next, game, state, player_id);
    }else{
        DecreaseOnlyHelper decrease_helper(*(next->workers_within_));
        // expend, then action randomly to get a quick result
        state = game.step(state, game.decodeAction(next->encoded_action_));
        while(!state.ended()){
            const auto& possible_action_encodes{game.getPossibleActionEncodes(state)};
            const auto next_encoded_action{possible_action_encodes.at(possible_action_encodes.size() * distribution(engine))};
            state = game.step(state, game.decodeAction(next_encoded_action));
        }
        result = std::make_pair(true, static_cast<std::size_t>(state.winner_));
        next->recordSuccess(result.second);
    }
    if(result.first) target->recordSuccess(result.second);
    return result;
}
Game::State MCTree::traceCurrentState(const Game& game)const{
    Game::State state{game.initializeState()};
    const MCTreeNode* target{&this->root_};
    while(target->guide_direction_ != nullptr){
        target = target->guide_direction_;
        state = game.step(state, game.decodeAction(target->encoded_action_));
    }
    if(target != this->current_root_){
        Logger::fatal(
            "invalid MCTree status: current root @", this->current_root_, " but direction from root led to ", target, "\n"
        );
        std::exit(EXIT_FAILURE);
    }
    return state;
}
void MCTree::__treeDump(const MCTreeNode* that)const{
    Logger::debug("node @ ", that, '\n');
    Logger::debug("action: ", that->encoded_action_, '\n');
    if(that->children_.size() > 0){
        Logger::debug("begin children of node @ ", that, '\n');
        for(const auto& item: that->children_) this->__treeDump(&item.second);
        Logger::debug("end children of node @ ", that, '\n');
    }else{
        Logger::debug("total visits: ", that->totalVisit(), '\n');
        for(std::size_t i = 0; i < that->validSucceedVisitNumber(); ++i){
            Logger::debug("succeed visit of player ", i, ": ", that->succeedVisit(i), '\n');
        }
    }
}
bool MCTree::step(MCTree* that, const Game& game, std::size_t player_id){
    Game::State state{game.initializeState()};
    return that->__step(&that->root_, game, state, player_id).first;
}
Game::Action MCTree::predict(const Game& game, std::size_t player_id){
    Logger::info("predicting optimal action form current root ", this->current_root_, '\n');
    MCTreeNode* result{nullptr};
    double best_winning_rate{0.0};
    for(auto& item: this->current_root_->children_){
        auto& child{item.second};
        auto temp_rate{child.winningRate(player_id)};
        if(result == nullptr || best_winning_rate < temp_rate){
            result = &child;
            best_winning_rate = temp_rate;
        }
    }
    if(result == nullptr){
        // current root have no children, which is not expected to happen
        Logger::error("argmax winning rate on current root returns no result with all ", this->current_root_->children_.size(), " children.\n");
        // generate a children randomly, just to move forward
        result = this->current_root_->expend(game, this->traceCurrentState(game));
        if(result == nullptr){
            // no way, it is even impossible for us to just generate a child randomly
            //  impossible to move on, we must exit
            Logger::fatal("failed to generate a random children, exiting\n");
            std::exit(EXIT_FAILURE);
        }
        Logger::info("winning rate: ???");
    }else{
        Logger::info("winning rate: ", best_winning_rate, '\n');
    }
    return game.decodeAction(result->encoded_action_);
}
void MCTree::move(std::size_t action, const Game& game){
    // save current root before move
    auto from{this->current_root_};
    // lookup for child with the required action
    const auto item{this->current_root_->children_.find(action)};
    if(item == this->current_root_->children_.end()){
        // no luck: that child does not exist!
        // we must create one for it
        Logger::info("action ", action, " for node ", this->current_root_, " is not visited, generating node for this\n");
        std::scoped_lock lock(*this->current_root_->children_mutex_);
        auto [iter, success]{this->current_root_->children_.try_emplace(action, game.setting_.player_count_, action)};
        --(*iter->second.workers_within_);
        this->current_root_ = &(iter->second);
    }else{
        // got it
        this->current_root_ = &(item->second);
    }
    Logger::debug("move from ", from, " to ", this->current_root_, '\n');
    // update the guide direction so that all up comming searches will find the right way
    from->guide_direction_ = this->current_root_;
    // send a message to saver to save nodes that will no longer be used in the current run
    //  and free them from memory
    this->command_queue_.put({SaveCommand::Command::ChangeRoot, from, this->current_root_, 0});
}
void MCTree::save(){
    this->command_queue_.put({SaveCommand::Command::SaveAll, this->current_root_, nullptr, 0});
}
void MCTree::treeDump()const{
    this->__treeDump(&this->root_);
}