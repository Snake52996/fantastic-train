_Pragma("once");
#include<includes/MCTreeNode.hpp>
#include<includes/game.hpp>
class MCTree{
  private:
    static constexpr double exploration_rate = 0.6;
    MCTreeNode* root;
    MCTreeNode* current_root;
    void __step(MCTreeNode* target, const Game& game)const;
  public:
    ~MCTree();
    static void step(const MCTree* that, const Game& game);
    std::size_t predict();
    void move(std::size_t action);
};