#include<includes/game.hpp>
#include<includes/MCTree.hpp>
#include<includes/Logger.hpp>
#include<includes/SimpleMessageQueue.hpp>
#include<includes/SaveCommand.hpp>
#include<includes/saver.hpp>
#include<includes/loader.hpp>
#include<iostream>
#include<thread>
#include<vector>
#include<atomic>
#include<semaphore>
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
    if(state.ended()){
        if(state.winner_ != Game::State::Dominator::None){
            output << state.winner_ << " wins\n";
        }else{
            output << "draw!\n";
        }
    }
}
template <class Rep, std::intmax_t num, std::intmax_t denom>
string formatChrono(std::chrono::duration<Rep, std::ratio<num, denom>> d){
    const auto hrs = chrono::duration_cast<chrono::hours>(d);
    const auto mins = chrono::duration_cast<chrono::minutes>(d - hrs);
    const auto secs = chrono::duration_cast<chrono::seconds>(d - hrs - mins);
    const auto ms = chrono::duration_cast<chrono::milliseconds>(d - hrs - secs);
    return to_string(hrs.count()) + 'h' + to_string(mins.count()) + 'm' + to_string(secs.count()) + 's' + to_string(ms.count()) + "ms";
}
int main(){
    static constexpr size_t mc_workers = 8;
    static constexpr auto allow_time = 2s;
    static constexpr auto allow_create_nodes = 1000000;
    Logger::setLogLevel(Logger::LogLevel::Info);
    counting_semaphore<> allowed_nodes(allow_create_nodes);
    Game game({3, 8, 5});
    SimpleMessageQueue<SaveCommand> command_queue;
    Saver saver{command_queue, allowed_nodes};
    command_queue.put({SaveCommand::Command::SavePlayerNumber, nullptr, nullptr, game.setting_.player_count_ + 1});
    Logger::info("launching...\n");
    Logger::info(mc_workers, " workers will be recruited\n");
    Logger::info("no more than ", allow_create_nodes, " nodes will be created\n");
    Logger::info("the AI player is allowed ", formatChrono(allow_time), " to plan each step\n");
    Logger::info("the board sizes ", game.setting_.dimension_size_, '^', game.setting_.dimension_count_, '\n');
    Logger::info("chain ", game.setting_.chain_to_win_, " locations to win the game\n");
    Logger::info("trying to load pretrained model...\n");
    auto loader{LoaderBuilder::build("MCTs")};
    unique_ptr<MCTree> mct;
    if(loader == nullptr){
        Logger::info("failed to load model\n");
        mct = make_unique<MCTree>(game, command_queue);
    }else{
        mct = make_unique<MCTree>(loader->load(), command_queue);
        Logger::info("model loaded\n");
    }
    Logger::info("launching saver\n");
    thread saver_thread{[&saver](){saver.run();}};
    Logger::info("saver ready\n");
    cout << "Select a side: 1 or odd for offensive, 2 or even for defensive: ";
    size_t helper;
    cin >> helper;
    auto ai_side{helper & 1 ? Game::State::Dominator::Player2 : Game::State::Dominator::Player1};
    Logger::info("recruiting workers\n");
    vector<thread> worker_threads;
    vector<bool> keep_worker;
    for(size_t i = 0; i < mc_workers; ++i){
        keep_worker.emplace_back(true);
        auto keep = keep_worker.back();
        worker_threads.emplace_back(thread{[&mct, &game, &allowed_nodes, keep, ai_side](){
            while(keep){
                if(allowed_nodes.try_acquire_for(125ms)){
                    if(!MCTree::step(mct.get(), game, static_cast<size_t>(ai_side))){
                        allowed_nodes.release();
                    }
                }
            }
        }});
    }
    Logger::info("workers in their place\n");
    Game::State state{game.initializeState()};
    Game::Action action{vector<size_t>(game.setting_.dimension_count_, 0)};
    while(!state.ended()){
        cout << state.next_ << '>';
        if(state.next_ == ai_side){
            this_thread::sleep_for(allow_time);
            action = mct->predict(game, static_cast<size_t>(ai_side));
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
        mct->move(game.encodeAction(action), game);
        printStateHelper(cout, game.setting_, state);
    }
    Logger::info("ready to exit, sending signals to terminate workers...\n");
    for(auto terminator: keep_worker) terminator = false;
    Logger::info("signals sent, waiting for workers to leave...\n");
    for(auto& worker: worker_threads) worker.join();
    Logger::info("all workers left, attemp to save rest information\n");
    mct->save();
    command_queue.put({SaveCommand::Command::Exit, nullptr, nullptr, 0});
    Logger::info("command sent, waiting for saver...\n");
    saver_thread.join();
    Logger::info("done, exiting\n");
    return 0;
}
