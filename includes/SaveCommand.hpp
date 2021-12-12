_Pragma("once");
#include<includes/MCTreeNode.hpp>
struct SaveCommand{
    enum class Command{
        SavePlayerNumber,
        ChangeRoot,
        SaveAll,
        Exit,
    };
    Command command;
    MCTreeNode* from;
    MCTreeNode* to;
    std::size_t number;
};