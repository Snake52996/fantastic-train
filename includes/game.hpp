_Pragma("once");
#include<vector>
#include<ostream>
class Game{
  public:
    struct State{
        enum class Dominator: unsigned short{
            Player1 = 0,
            Player2,
            None
        };
        Dominator winner_;
        Dominator next_;
        std::size_t occupied_;
        std::vector<Dominator> flatten_board_;
        State(std::size_t total_locations);
        bool ended()const;
        Dominator nextPlayer()const;
    };
    struct Action{
        std::vector<size_t> target_;
    };
    struct Settings{
        constexpr static std::size_t player_count_ = static_cast<std::size_t>(Game::State::Dominator::None);
        unsigned short dimension_count_;
        std::size_t dimension_size_;
        std::size_t chain_to_win_;
        std::vector<std::size_t> dimension_batch_;
        std::size_t total_locations_;
        Settings(unsigned short dimension_count, std::size_t dimension_size, std::size_t chain_to_win);
        std::size_t getFlattenIndex(const std::vector<size_t>& structured_index)const;
        std::vector<std::size_t> getStructuredIndex(std::size_t flatten_index)const;
    };
    const Settings setting_;
    Game(Settings setting);
    State initializeState()const;
    std::size_t encodeAction(const Action& action)const;
    Action decodeAction(std::size_t encode)const;
    Action diffAction(const State& from, const State& to)const;
    std::vector<std::size_t> getPossibleActionEncodes(const State& state)const;
    State step(const State& current_state, const Action& current_action)const;
};
std::ostream& operator<<(std::ostream& lhs, const Game::State::Dominator& rhs);