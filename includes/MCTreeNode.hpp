_Pragma("once");
#include<includes/game.hpp>
#include<map>
#include<forward_list>
#include<atomic>
#include<vector>
class MCTreeNode{
  private:
    bool finalized_;
    std::atomic_size_t total_simulation_;
    std::atomic_size_t succeed_simulation_;
  public:
    Game::State state_;
    std::map<std::size_t, MCTreeNode> children_;
    MCTreeNode(const Game::State& state);
    ~MCTreeNode() = default;
    void finalize();
    bool finalized()const;
    std::size_t totalVisit()const;
    double winningRate()const;
    double UCT(const std::size_t global_simulation, const double lambda)const;
    void recordSuccess();
    void recordFailure();
    const MCTreeNode* select(const std::size_t global_simulation, const double lambda)const;
    MCTreeNode* expend(const std::vector<std::size_t>& possible_actions, const Game& game);
};