#include<includes/MCTreeNode.hpp>
#include<cmath>
#include<random>
#include<algorithm>
#include<iterator>
MCTreeNode::MCTreeNode(const Game::State& state): finalized_(false), total_simulation_(0), succeed_simulation_(0), state_(state){}
void MCTreeNode::finalize(){
    this->finalized_ = true;
}
bool MCTreeNode::finalized()const{
    return this->finalized_;
}
std::size_t MCTreeNode::totalVisit()const{
    return this->total_simulation_;
}
double MCTreeNode::winningRate()const{
    return static_cast<double>(succeed_simulation_) / static_cast<double>(total_simulation_);
}
double MCTreeNode::UCT(const std::size_t global_simulation, const double lambda)const{
    return this->winningRate() + lambda * sqrt(
        std::log(static_cast<double>(global_simulation)) / static_cast<double>(this->total_simulation_)
    );
}
void MCTreeNode::recordSuccess(){
    ++this->total_simulation_;
    ++this->succeed_simulation_;
}
void MCTreeNode::recordFailure(){
    ++this->total_simulation_;
}
const MCTreeNode* MCTreeNode::select(const std::size_t global_simulation, const double lambda)const{
    const MCTreeNode* result = nullptr;
    double max_result = 0;
    for(auto& item: this->children_){
        auto& child = item.second;
        double temp_result = child.UCT(global_simulation, lambda);
        if(result == nullptr || temp_result > max_result){
            result = &child;
            max_result = temp_result;
        }
    }
    return result;
}
MCTreeNode* MCTreeNode::expend(const std::vector<std::size_t>& possible_actions, const Game& game){
    if(possible_actions.size() <= this->children_.size()) return nullptr;
    std::vector<std::size_t> selectable_actions;
    std::copy_if(
        possible_actions.begin(), possible_actions.end(),
        std::back_inserter(selectable_actions), [this](const std::size_t& action)->bool{
            return !this->children_.contains(action);
        }
    );
    if(selectable_actions.size() == 0) return nullptr;
    std::random_device rd;
    std::mt19937_64 engine(rd());
    std::uniform_int_distribution<std::size_t> distribute(0, selectable_actions.size() - 1);
    std::size_t target{selectable_actions.at(distribute(engine))};
    auto [iter, succeed]{this->children_.emplace(target, game.step(this->state_, game.decodeAction(target)))};
    if(succeed) return &iter->second;
    else return nullptr;
}