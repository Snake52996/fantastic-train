_Pragma("once");
#include<includes/MCTreeNode.hpp>
#include<fstream>
#include<filesystem>
#include<memory>
class Loader{
  protected:
    std::ifstream input_;
    Loader(std::ifstream&& input);
    void setTotalSimulation(MCTreeNode& target, std::size_t value);
    void setSucceedSimulation(MCTreeNode& target, std::size_t id, std::size_t value);
    void addTotalSimulation(MCTreeNode& target, std::size_t value);
    void addSucceedSimulation(MCTreeNode& target, std::size_t id, std::size_t value);
  public:
    virtual ~Loader() = default;
    virtual MCTreeNode load() = 0;
};
namespace LoaderBuilder{
    std::unique_ptr<Loader> build(const std::filesystem::path& path);
}