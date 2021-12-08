#include<includes/MCTree.hpp>
#include<cmath>
#include<random>
void MCTree::__step(MCTreeNode* target, const Game& game)const{
    std::random_device rd;
    std::mt19937_64 engine(rd());
    std::uniform_real_distribution distribution(0.0, 1.0);
    if(distribution(engine) <= MCTree::exploration_rate){
        
    }
}
MCTree::~MCTree(){
    delete this->root;
}
void MCTree::step(const MCTree* that, const Game& game){
    that->__step(that->current_root, game);
}
std::size_t MCTree::predict(){
    MCTreeNode* result;
    double best_winning_rate = std::nan("");
    for(const auto& item: this->current_root->children_){
        const auto& child{item.second};
        auto temp_rate{child->winningRate()};
        if(std::isnan(best_winning_rate) || best_winning_rate < temp_rate){
            result = child;
            best_winning_rate = temp_rate;
        }
    }
    this->current_root = result;
    return result->action_;
}
void MCTree::move(std::size_t action){
    const auto& item{this->current_root->children_.find(action)};
    if(item == this->current_root->children_.end()){
        auto target_node{new MCTreeNode()};
        target_node->action_ = action;
        this->current_root->children_.insert({action, target_node});
        this->current_root = target_node;
    }else{
        this->current_root = item->second;
    }
}