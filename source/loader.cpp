#include<includes/Logger.hpp>
#include<includes/loader.hpp>
Loader::Loader(std::ifstream&& input): input_(std::move(input)){}
void Loader::setTotalSimulation(MCTreeNode& target, std::size_t value){
    *target.total_simulation_ = value;
}
void Loader::setSucceedSimulation(MCTreeNode& target, std::size_t id, std::size_t value){
    *target.succeed_simulation_.at(id) = value;
}
void Loader::addTotalSimulation(MCTreeNode& target, std::size_t value){
    *target.total_simulation_ += value;
}
void Loader::addSucceedSimulation(MCTreeNode& target, std::size_t id, std::size_t value){
    *target.succeed_simulation_.at(id) += value;
}
namespace{
    template<std::integral T>
    T readInteger(std::istream& input){
        // require little endian to work
        static_assert(std::endian::native == std::endian::little);
        static constexpr char MaxBytes = 8;
        char helper_buffer{0};
        // get first byte
        input.read(&helper_buffer, 1);
        if(helper_buffer == 0){
            //Logger::debug("read 0\n");
            return static_cast<T>(0);
        }
        if(helper_buffer <= MaxBytes && helper_buffer >= -MaxBytes){
            // read further bytes
            T result{0};
            bool negative{helper_buffer < 0};
            int bytes_required{negative ? -helper_buffer : helper_buffer};
            input.read(reinterpret_cast<char*>(&result), bytes_required);
            //Logger::debug("read ", negative ? ~result : result, '\n');
            return negative ? ~result : result;
        }else{
            // get the value directly
            if(helper_buffer < 0){
                //Logger::debug("read ", static_cast<T>(helper_buffer + MaxBytes), '\n');
                return static_cast<T>(helper_buffer + MaxBytes);
            }else{
                //Logger::debug("read ", static_cast<T>(helper_buffer - MaxBytes), '\n');
                return static_cast<T>(helper_buffer - MaxBytes);
            }
        }
    }
    class V0Loader: public Loader{
      private:
        void insertOne(std::map<std::size_t, MCTreeNode>& target, std::size_t player_count){
            auto action{readInteger<std::size_t>(this->input_)};
            //Logger::debug("action: ", action, '\n');
            auto [iter, success] = target.try_emplace(action, player_count - 1, action);
            // this method always runs in single thread mode, no protection is required
            //  the counter must be cleared to avoid deadlock in later sequences
            --(*iter->second.workers_within_);
            auto children_count{readInteger<std::size_t>(this->input_)};
            //Logger::debug("children count: ", children_count, '\n');
            if(children_count == 0){
                this->setTotalSimulation(iter->second, readInteger<std::size_t>(this->input_));
                for(std::size_t i = 0; i < player_count; ++i){
                    this->setSucceedSimulation(iter->second, i, readInteger<std::size_t>(this->input_));
                }
            }else{
                for(std::size_t i = 0; i < children_count; ++i) this->insertOne(iter->second.children_, player_count);
                for(const auto& [key, node]: iter->second.children_){
                    this->addTotalSimulation(iter->second, node.totalVisit());
                    for(std::size_t j = 0; j < player_count; ++j){
                        this->addSucceedSimulation(iter->second, j, node.succeedVisit(j));
                    }
                }
            }
        }
      public:
        V0Loader(std::ifstream&& input): Loader(std::move(input)){}
        MCTreeNode load()override{
            // get number of players
            auto player_count{readInteger<std::size_t>(this->input_)};
            //Logger::debug("player count: ", player_count, '\n');
            // prepare root node
            MCTreeNode result(player_count - 1, readInteger<std::size_t>(this->input_));
            --(*result.workers_within_);
            auto children_count{readInteger<std::size_t>(this->input_)};
            //Logger::debug("children of root: ", children_count, '\n');
            if(children_count == 0){
                this->setTotalSimulation(result, readInteger<std::size_t>(this->input_));
                for(std::size_t i = 0; i < player_count; ++i){
                    this->setSucceedSimulation(result, i, readInteger<std::size_t>(this->input_));
                }
            }else{
                for(std::size_t i = 0; i < children_count; ++i) this->insertOne(result.children_, player_count);
                for(const auto& [key, node]: result.children_){
                    this->addTotalSimulation(result, node.totalVisit());
                    for(std::size_t j = 0; j < player_count; ++j){
                        this->addSucceedSimulation(result, j, node.succeedVisit(j));
                    }
                }
            }
            // use helper to finish the load
            return result;
        }
    };
}
std::unique_ptr<Loader> LoaderBuilder::build(const std::filesystem::path& path){
    // check if the file exists and is a regular file
    if(!std::filesystem::is_regular_file(path)){
        Logger::error("direction of path ", path.c_str(), " does not exist or is not a regular file\n");
        return std::unique_ptr<Loader>(nullptr);
    }
    // identify the magic number to determain loader version to use
    std::size_t magic_number;
    std::ifstream input(path, std::ios::binary);
    input.read(reinterpret_cast<char*>(&magic_number), sizeof(std::size_t));
    if(input.gcount() != sizeof(std::size_t)){
        // something is wrong, we can not read the magic number
        Logger::error("failed to read magic number from ", path.c_str(), '\n');
        return std::unique_ptr<Loader>(nullptr);
    }
    switch(magic_number){
        case 7097568816874001UL:
            return std::make_unique<V0Loader>(std::move(input));
        default:
            Logger::error("no matched loader for magic number ", magic_number, '\n');
            return std::unique_ptr<Loader>(nullptr);
    }
}