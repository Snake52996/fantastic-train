_Pragma("once");
#include<includes/MCTreeNode.hpp>
struct SaveCommand{
    enum class Command{
        ChangeRoot,
        SaveAll,
        Exit,
    };
    Command command;
    MCTreeNode* from;
    MCTreeNode* to;
};