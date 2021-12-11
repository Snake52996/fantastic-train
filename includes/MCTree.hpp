_Pragma("once");
#include<includes/MCTreeNode.hpp>
#include<includes/game.hpp>
#include<includes/SimpleMessageQueue.hpp>
#include<includes/SaveCommand.hpp>
#include<utility>
#include<vector>
class MCTree{
  private:
    static constexpr double exploration_rate = 0.6;
    static constexpr double lambda = 1.414;
    MCTreeNode root_;
    MCTreeNode* current_root_;
    SimpleMessageQueue<SaveCommand>& command_queue_;
    std::pair<bool, std::size_t> __step(MCTreeNode* target, const Game& game, Game::State& state, std::size_t player_id)const;
    Game::State traceCurrentState(const Game& game)const;
  public:
    MCTree(const Game& game, SimpleMessageQueue<SaveCommand>& command_queue);
    ~MCTree();
    static void step(MCTree* that, const Game& game, std::size_t player_id);
    Game::Action predict(const Game& game, std::size_t player_id);
    void move(std::size_t action, const Game& game);
    void save();
};