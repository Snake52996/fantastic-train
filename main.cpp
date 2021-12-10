#include<includes/game.hpp>
#include<includes/MCTree.hpp>
#include<includes/Logger.hpp>
#include<iostream>
#include<thread>
#include<vector>
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
    static constexpr size_t mc_workers = 2;
    static constexpr auto allow_time = 5s;
    Logger::setLogLevel(Logger::LogLevel::Warning);
    Game game({2, 6, 4});
    MCTree mct{Game::State::Dominator::Player2, game};
    vector<thread> worker_threads;
    for(size_t i = 0; i < mc_workers; ++i){
        worker_threads.emplace_back(thread{[&mct, &game](){
            while(true) MCTree::step(&mct, game);
        }});
    }
    Game::State state{game.initializeState()};
    Game::Action action{vector<size_t>(game.setting_.dimension_count_, 0)};
    size_t helper;
    while(!state.ended()){
        cout << state.next_ << '>';
        if(state.next_ == Game::State::Dominator::Player2){
            this_thread::sleep_for(allow_time);
            action = mct.predict(game);
            for(const auto& index: action.target_) cout << ' ' << index;
            cout << '\n';
        }else{
            cout << ' ';
            for(size_t i = 0; i < game.setting_.dimension_count_; ){
                cin >> helper;
                if(helper < game.setting_.dimension_size_){
                    action.target_.at(i++) = helper;
                }
            }
        }
        state = game.step(state, action);
        mct.move(game.setting_.getFlattenIndex(action.target_), game);
        printStateHelper(cout, game.setting_, state);
    }
    return 0;
}