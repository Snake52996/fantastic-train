#include<includes/MCTreeNode.hpp>
#include<cmath>
#include<random>
MCTreeNode::MCTreeNode(): state_(nullptr), total_simulation_(0), succeed_simulation_(0){}
double MCTreeNode::winningRate()const{
    return static_cast<double>(succeed_simulation_) / static_cast<double>(total_simulation_);
}
MCTreeNode::~MCTreeNode(){
    if(this->state_ != nullptr) delete this->state_;
    for(auto& item: this->children_) delete item.second;
}
double MCTreeNode::UCT(const std::size_t global_simulation, const double lambda)const{
    return this->winningRate() + lambda * sqrt(
        std::log(static_cast<double>(global_simulation)) / static_cast<double>(this->total_simulation_)
    );
}
MCTreeNode* MCTreeNode::select(const std::size_t global_simulation, const double lambda)const{
    MCTreeNode* result = nullptr;
    double max_result = 0;
    for(const auto& item: this->children_){
        auto& child = item.second;
        double temp_result = child->UCT(global_simulation, lambda);
        if(result == nullptr || temp_result > max_result){
            result = child;
            max_result = temp_result;
        }
    }
    return result;
}
MCTreeNode* MCTreeNode::expend(const std::size_t min_encode, const std::size_t max_encode){
    std::random_device rd;
    std::mt19937_64 engine(rd());
    std::uniform_int_distribution<std::size_t> distribute(min_encode, max_encode);
    MCTreeNode* result = nullptr;
    while(true){
        std::size_t new_target = distribute(engine);
        if(!this->children_.contains(new_target)){
            result = new MCTreeNode();
            result->action_ = new_target;
            this->children_.insert({new_target, result});
            break;
        }
    }
    return result;
}