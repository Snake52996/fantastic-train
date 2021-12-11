#include<includes/MCTreeNode.hpp>
#include<includes/Logger.hpp>
#include<cmath>
#include<random>
#include<algorithm>
#include<iterator>
MCTreeNode::MCTreeNode(
    std::size_t player_number, std::size_t encoded_action
): total_simulation_(0), encoded_action_(encoded_action), guide_direction_(nullptr), workers_within_(0){
    for(std::size_t i = 0; i < player_number + 1; ++i) this->succeed_simulation_.emplace_back(new std::atomic_size_t(0));
    Logger::debug("C: succeed_simulation_ @", this , " have size ", this->succeed_simulation_.size(), '\n');
}
std::size_t MCTreeNode::totalVisit()const{
    return this->total_simulation_;
}
std::size_t MCTreeNode::validSucceedVisitNumber()const{
    return this->succeed_simulation_.size();
}
std::size_t MCTreeNode::succeedVisit(std::size_t id)const{
    Logger::debug("succeed_simulation_ @", this , " have size ", this->succeed_simulation_.size(), '\n');
    return *this->succeed_simulation_.at(id);
}
double MCTreeNode::winningRate(std::size_t player_id)const{
    return static_cast<double>(this->succeedVisit(player_id)) / static_cast<double>(total_simulation_);
}
double MCTreeNode::UCT(std::size_t player_id, const std::size_t global_simulation, const double lambda)const{
    return this->winningRate(player_id) + lambda * sqrt(
        std::log(static_cast<double>(global_simulation)) / static_cast<double>(this->total_simulation_)
    );
}
void MCTreeNode::recordGame(){
    ++this->total_simulation_;
}
void MCTreeNode::recordSuccess(std::size_t player_id){
    this->recordGame();
    ++(*this->succeed_simulation_.at(player_id));
}
MCTreeNode* MCTreeNode::expend(const Game& game, const Game::State& state){
    auto possible_actions{game.getPossibleActionEncodes(state)};
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
    std::scoped_lock lock(this->children_mutex_);
    Logger::trace("expending action ", target, " for node ", this, '\n');
    auto [iter, succeed]{this->children_.try_emplace(target, game.setting_.player_count_, target)};
    Logger::trace("done with node ", this, '\n');
    if(succeed) return &iter->second;
    else return nullptr;
}
