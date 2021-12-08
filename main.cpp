#include<includes/game.hpp>
#include<iostream>
using namespace std;
void printStateHelper(ostream& output, const Game::Settings& setting, const Game::State& state){
    output << "========================================";
    for(size_t i = 0; i < state.flatten_board_.size(); ++i) if(state.flatten_board_.at(i) != Game::State::Dominator::None){
        auto structured_index{setting.getStructuredIndex(i)};
        output << '\n' << state.flatten_board_.at(i) << "@(";
        bool space_helper{false};
        for(const auto& index: structured_index){
            if(space_helper) output << ", ";
            else space_helper = true;
            output << index;
        }
        output << ')';
    }
    output << "\n========================================\n";
    if(state.winner_ != Game::State::Dominator::None){
        output << state.winner_ << " wins";
    }
}
int main(){
    Game game({3, 10, 3});
    Game::State state{game.initializeState()};
    Game::Action action{Game::Action::Operator::Player1, vector<size_t>(game.setting_.dimension_count_, 0)};
    size_t helper;
    while(state.winner_ == Game::State::Dominator::None){
        action.actor_ = fromDominator(state.next_);
        cout << state.next_ << "> ";
        for(size_t i = 0; i < game.setting_.dimension_count_; ){
            cin >> helper;
            if(helper < game.setting_.dimension_size_){
                action.target_.at(i++) = helper;
            }
        }
        state = game.step(state, action);
        printStateHelper(cout, game.setting_, state);
    }
    return 0;
}