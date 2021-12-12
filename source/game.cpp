#include<includes/BitVector.hpp>
#include<includes/game.hpp>
#include<includes/Logger.hpp>
#include<iterator>
Game::State::State(
    std::size_t total_locations
): winner_(Dominator::None), next_(Dominator::Player1), occupied_(0), flatten_board_(total_locations, Dominator::None){}
bool Game::State::ended()const{
    return (this->winner_ != Dominator::None) || (this->next_ == Dominator::None);
}
Game::State::Dominator Game::State::nextPlayer()const{
    static constexpr auto limit_player = static_cast<unsigned short>(Dominator::None);
    return static_cast<Dominator>((static_cast<unsigned short>(this->next_) + 1) % limit_player);
}
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
std::size_t Game::encodeAction(const Game::Action& action)const{
    return this->setting_.getFlattenIndex(action.target_);
}
Game::Action Game::decodeAction(std::size_t encode)const{
    return Action{this->setting_.getStructuredIndex(encode)};
}
Game::Action Game::diffAction(const Game::State& from, const Game::State& to)const{
    for(size_t i = 0; i < from.flatten_board_.size(); ++i) if(from.flatten_board_.at(i) != to.flatten_board_.at(i)){
        return this->decodeAction(i);
    }
    return this->decodeAction(0);
}
std::vector<std::size_t> Game::getPossibleActionEncodes(const State& state)const{
    // Logger::debug("find possible actions for state:\n");
    // for(std::size_t i = 0; i < 4; ++i){
    //     Logger::debug(state.flatten_board_.at(i << 2), ' ', state.flatten_board_.at((i << 2) | 1), ' ', state.flatten_board_.at((i << 2) | 2), ' ', state.flatten_board_.at((i << 2) | 3), '\n');
    // }
    std::vector<std::size_t> result;
    if(state.ended()) return result;
    for(std::size_t i = 0; i < state.flatten_board_.size(); ++i) if(state.flatten_board_.at(i) == State::Dominator::None){
        result.emplace_back(i);
    }
    // Logger::debug("result is\n");
    // for(const auto item: result) Logger::debug(item, '\n');
    return result;
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
    std::size_t target_offset{this->setting_.getFlattenIndex(current_action.target_)};
    if(current_state.flatten_board_.at(target_offset) != State::Dominator::None) return current_state;
    State result{current_state};
    result.flatten_board_.at(target_offset) = current_state.next_;
    ++result.occupied_;
    BitVector iteration_helper{this->setting_.dimension_count_};
    iteration_helper.next();
    bool finished = false;
    do{
        std::size_t total_count = 1;
        scanSide(total_count, true, current_action.target_, iteration_helper, result, current_state.next_);
        scanSide(total_count, false, current_action.target_, iteration_helper, result, current_state.next_);
        if(total_count >= this->setting_.chain_to_win_){
            finished = true;
            break;
        }
    }while(iteration_helper.next());
    if(finished){
        result.winner_ = current_state.next_;
        result.next_ = State::Dominator::None;
    }else{
        if(result.occupied_ < this->setting_.total_locations_){
            result.next_ = current_state.nextPlayer();
        }else{
            result.next_ = State::Dominator::None;
        }
    }
    return result;
}
std::ostream& operator<<(std::ostream& lhs, const Game::State::Dominator& rhs){
    switch(rhs){
        case Game::State::Dominator::None:
            lhs << 'X';
            break;
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
