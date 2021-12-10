#include<includes/MCTree.hpp>
#include<includes/Logger.hpp>
#include<cmath>
#include<random>
#include<cstdlib>
#include<string>
#include<thread>
#include<mutex>
MCTree::MCTree(Game::State::Dominator view, const Game& game): view_(view), root_(nullptr), current_root_(nullptr){
    this->root_ = new MCTreeNode(game.initializeState());
    if(this->root_ == nullptr){
        Logger::fatal("failed to allocate memory for MCTree root node\n");
        std::exit(EXIT_FAILURE);
    }
    this->current_root_ = this->root_;
}
MCTree::~MCTree(){
    delete this->root_;
}
std::pair<bool, bool> MCTree::__step(MCTreeNode* target, const Game& game)const{
    if(target->finalized()) return std::make_pair(false, false);
    std::random_device rd;
    std::mt19937_64 engine(rd());
    std::uniform_real_distribution distribution(0.0, 1.0);
    std::pair<bool, bool> result;
    MCTreeNode* next{nullptr};
    bool expand_only{target->children_.size() == 0};
    bool expand{expand_only || distribution(engine) <= MCTree::exploration_rate};
    if(
        (!expand) ||  (next = target->expend(game.getPossibleActionEncodes(target->state_), game)) == nullptr
    ){
        if(expand_only) return std::make_pair(false, false);
        double best_reward{0.0};
        for(auto& item: target->children_){
            auto temp_reward{item.second.UCT(this->current_root_->totalVisit(), MCTree::lambda)};
            if(next == nullptr || temp_reward > best_reward){
                next = &(item.second);
                best_reward = temp_reward;
            }
        }
        result = this->__step(next, game);
    }else{
        auto current_state{next->state_};
        if(current_state.ended()) next->finalize();
        while(!current_state.ended()){
            const auto& possible_action_encodes{game.getPossibleActionEncodes(current_state)};
            const auto next_encoded_action{possible_action_encodes.at(possible_action_encodes.size() * distribution(engine))};
            current_state = game.step(current_state, game.decodeAction(next_encoded_action));
        }
        result = std::make_pair(current_state.winner_ != Game::State::Dominator::None, current_state.winner_ == this->view_);
        if(result.first){
            if(result.second){
                next->recordSuccess();
            }else{
                next->recordFailure();
            }
        }
    }
    if(result.first){
        if(result.second) target->recordSuccess();
        else target->recordFailure();
    }
    return result;
}
void MCTree::step(const MCTree* that, const Game& game){
    that->__step(that->current_root_, game);
}
Game::Action MCTree::predict(const Game& game){
    MCTreeNode* result{nullptr};
    double best_winning_rate;
    for(auto& item: this->current_root_->children_){
        auto& child{item.second};
        auto temp_rate{child.winningRate()};
        if(result == nullptr || best_winning_rate < temp_rate){
            result = &child;
            best_winning_rate = temp_rate;
        }
    }
    if(result == nullptr){
        // current root have no children, which is not expected to happen
        Logger::error("argmax winning rate on current root returns no result with all ", this->current_root_->children_.size(), " children.\n");
        // generate a children randomly, just to move forward
        result = this->current_root_->expend(game.getPossibleActionEncodes(this->current_root_->state_), game);
        if(result == nullptr){
            // no way, it is even impossible for us to just generate a child randomly
            //  impossible to move on, we must exit
            Logger::fatal("failed to generate a random children, exiting\n");
            std::exit(EXIT_FAILURE);
        }
    }
    MCTreeNode* save_current_root = this->current_root_;
    this->current_root_ = result;
    return game.diffAction(save_current_root->state_, this->current_root_->state_);
}
void MCTree::move(std::size_t action, const Game& game){
    const auto& item{this->current_root_->children_.find(action)};
    if(item == this->current_root_->children_.end()){
        std::scoped_lock lock(this->current_root_->children_mutex_);
        auto [iter, success]{this->current_root_->children_.emplace(
            action, game.step(current_root_->state_, game.decodeAction(action))
        )};
        this->current_root_ = &iter->second;
    }else{
        this->current_root_ = &item->second;
    }
}
