_Pragma("once");
#include<includes/game.hpp>
#include<map>
#include<forward_list>
#include<atomic>
class MCTreeNode{
  private:
    std::atomic_size_t total_simulation_;
    std::atomic_size_t succeed_simulation_;
  public:
    Game::State* state_;
    std::size_t action_;
    std::map<std::size_t, MCTreeNode*> children_;
    MCTreeNode();
    ~MCTreeNode();
    double winningRate()const;
    double UCT(const std::size_t global_simulation, const double lambda)const;
    MCTreeNode* select(const std::size_t global_simulation, const double lambda)const;
    MCTreeNode* expend(const std::size_t min_encode, const std::size_t max_encode);
};