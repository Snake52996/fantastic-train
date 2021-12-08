_Pragma("once");
#include<vector>
#include<ostream>
class Game{
  public:
    struct State{
        enum class Dominator: unsigned short{
            None,
            Player1,
            Player2,
        };
        Dominator winner_;
        Dominator next_;
        std::vector<Dominator> flatten_board_;
        State(std::size_t total_locations);
    };
    struct Action{
        enum class Operator: unsigned short{
            Player1 = 0,
            Player2,
            PlayerLimit
        };
        Operator actor_;
        std::vector<size_t> target_;
    };
    struct Settings{
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
    State step(const State& current_state, const Action& current_action)const;
};
std::ostream& operator<<(std::ostream& lhs, const Game::State::Dominator& rhs);
Game::State::Dominator fromOperator(const Game::Action::Operator& source);
Game::Action::Operator fromDominator(const Game::State::Dominator& source);
Game::Action::Operator nextPlayer(const Game::Action::Operator& source);