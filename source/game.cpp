#include<includes/BitVector.hpp>
#include<includes/game.hpp>
#include<iterator>
Game::State::State(
    std::size_t total_locations
): winner_(Dominator::None), next_(Dominator::Player1), flatten_board_(total_locations, Dominator::None){}
Game::Settings::Settings(
    unsigned short dimension_count, std::size_t dimension_size, std::size_t chain_to_win
): dimension_count_(dimension_count), dimension_size_(dimension_size), chain_to_win_(chain_to_win){
    this->dimension_batch_.resize(dimension_count);
    dimension_batch_.back() = 1;
    for(auto iter = std::next(this->dimension_batch_.rbegin()); iter != this->dimension_batch_.rend(); ++iter){
        *iter = *std::prev(iter) * dimension_size;
    }
    this->total_locations_ = this->dimension_batch_.front() * dimension_size;
}
std::size_t Game::Settings::getFlattenIndex(const std::vector<size_t>& structured_index)const{
    if(structured_index.size() != static_cast<std::size_t>(this->dimension_count_)) return 0;
    std::size_t result = 0;
    for(size_t i = 0; i < structured_index.size(); ++i){
        result += this->dimension_batch_.at(i) * structured_index.at(i);
    }
    return result;
}
std::vector<std::size_t> Game::Settings::getStructuredIndex(std::size_t flatten_index)const{
    std::vector<std::size_t> result;
    for(const auto& batch: this->dimension_batch_){
        result.emplace_back(flatten_index / batch);
        flatten_index %= batch;
    }
    return result;
}
Game::Game(Settings setting): setting_(setting){}
Game::State Game::initializeState()const{
    return State(this->setting_.total_locations_);
}
Game::State Game::step(const State& current_state, const Action& current_action)const{
    static auto indexModifier = [](
        std::vector<std::size_t>& index, bool negative, std::size_t boundary, const BitVector& mask
    )->bool{
        for(std::size_t i = 0 ; i < index.size(); ++i) if(mask.data_.at(i)){
            if(index.at(i) == boundary) return false;
            if(negative) --index.at(i);
            else ++index.at(i);
        }
        return true;
    };
    static auto scanSide = [this](
        std::size_t& total,
        bool negative,
        const std::vector<std::size_t>& origin,
        const BitVector& mask,
        const State& state,
        const State::Dominator target
    ){
        if(total >= this->setting_.chain_to_win_) return;
        auto index{origin};
        while(
            indexModifier(index, negative, negative ? 0 : this->setting_.dimension_size_ - 1, mask) &&
            total < this->setting_.chain_to_win_
        ){
            if(state.flatten_board_.at(this->setting_.getFlattenIndex(index)) == target){
                ++total;
            }else{
                break;
            }
        }
    };
    if(current_state.winner_ != State::Dominator::None) return current_state;
    State::Dominator current_dominator{fromOperator(current_action.actor_)};
    if(current_state.next_ != current_dominator) return current_state;
    std::size_t target_offset{this->setting_.getFlattenIndex(current_action.target_)};
    if(current_state.flatten_board_.at(target_offset) != State::Dominator::None) return current_state;
    State result{current_state};
    result.flatten_board_.at(target_offset) = current_dominator;
    BitVector iteration_helper{this->setting_.dimension_count_};
    iteration_helper.next();
    bool finished = false;
    do{
        std::size_t total_count = 1;
        scanSide(total_count, true, current_action.target_, iteration_helper, result, current_dominator);
        scanSide(total_count, false, current_action.target_, iteration_helper, result, current_dominator);
        if(total_count >= this->setting_.chain_to_win_){
            finished = true;
            break;
        }
    }while(iteration_helper.next());
    if(finished){
        result.winner_ = current_dominator;
        result.next_ = State::Dominator::None;
    }else{
        result.next_ = fromOperator(nextPlayer(current_action.actor_));
    }
    return result;
}
std::ostream& operator<<(std::ostream& lhs, const Game::State::Dominator& rhs){
    switch(rhs){
        case Game::State::Dominator::Player1:
            lhs << 'B';
            break;
        case Game::State::Dominator::Player2:
            lhs << 'W';
            break;
        default:
            break;
    }
    return lhs;
}
Game::State::Dominator fromOperator(const Game::Action::Operator& source){
    switch(source){
        case Game::Action::Operator::Player1:
            return Game::State::Dominator::Player1;
        case Game::Action::Operator::Player2:
            return Game::State::Dominator::Player2;
        default:
            return Game::State::Dominator::Player1;
    }
}
Game::Action::Operator fromDominator(const Game::State::Dominator& source){
    switch(source){
        case Game::State::Dominator::Player1:
            return Game::Action::Operator::Player1;
        case Game::State::Dominator::Player2:
            return Game::Action::Operator::Player2;
        default:
            return Game::Action::Operator::Player1;
    }
}
Game::Action::Operator nextPlayer(const Game::Action::Operator& source){
    static auto limit_player{static_cast<unsigned short>(Game::Action::Operator::PlayerLimit)};
    auto current_player{static_cast<unsigned short>(source)};
    return static_cast<Game::Action::Operator>((current_player + 1) % limit_player);
}