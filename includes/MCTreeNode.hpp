_Pragma("once");
class MCTreeNode;
#include<includes/game.hpp>
#include<includes/loader.hpp>
#include<map>
#include<atomic>
#include<vector>
#include<mutex>
#include<memory>
class MCTreeNode{
  private:
    std::unique_ptr<std::atomic_size_t> total_simulation_;
    std::vector<std::atomic_size_t*> succeed_simulation_;
  public:
    std::size_t encoded_action_;
    std::map<std::size_t, MCTreeNode> children_;
    std::unique_ptr<std::mutex> children_mutex_;
    MCTreeNode* guide_direction_;
    std::unique_ptr<std::atomic_size_t> workers_within_;
    MCTreeNode(std::size_t player_number, std::size_t encoded_action);
    MCTreeNode(MCTreeNode&& other);
    ~MCTreeNode();
    std::size_t totalVisit()const;
    std::size_t validSucceedVisitNumber()const;
    std::size_t succeedVisit(std::size_t id)const;
    double winningRate(std::size_t player_id)const;
    double UCT(std::size_t player_id, const std::size_t global_simulation, const double lambda)const;
    void recordGame();
    void recordSuccess(std::size_t player_id);
    MCTreeNode* expend(const Game& game, const Game::State& state);
    friend class Loader;
};