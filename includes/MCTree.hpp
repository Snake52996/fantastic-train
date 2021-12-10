_Pragma("once");
#include<includes/MCTreeNode.hpp>
#include<includes/game.hpp>
#include<utility>
#include<vector>
class MCTree{
  private:
    static constexpr double exploration_rate = 0.6;
    static constexpr double lambda = 1.414;
    const Game::State::Dominator view_;
    MCTreeNode* root_;
    MCTreeNode* current_root_;
    std::pair<bool, bool> __step(MCTreeNode* target, const Game& game)const;
  public:
    MCTree(Game::State::Dominator view, const Game& game);
    ~MCTree();
    static void step(const MCTree* that, const Game& game);
    Game::Action predict(const Game& game);
    void move(std::size_t action, const Game& game);
};