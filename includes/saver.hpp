_Pragma("once");
#include<includes/MCTreeNode.hpp>
#include<includes/SimpleMessageQueue.hpp>
#include<includes/SaveCommand.hpp>
#include<fstream>
class Saver{
  private:
    constexpr static std::size_t MagicNumber = 7097568816874001UL;
    SimpleMessageQueue<SaveCommand>& command_queue_;
    std::ofstream file_;
    void waitForWorkers(MCTreeNode* target)const;
    void saveOneUnlocked(MCTreeNode* target);
    void saveAllUnlocked(MCTreeNode* target);
    void saveAll(MCTreeNode* target);
    void saveChangeRoot(MCTreeNode* from, MCTreeNode* to);
  public:
    Saver(SimpleMessageQueue<SaveCommand>& command_queue);
    void run();
};