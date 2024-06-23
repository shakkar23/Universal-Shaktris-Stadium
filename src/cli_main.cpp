#include <fstream>
#include <iostream>

#include "Bot.hpp"
#include "VersusGame.hpp"
#include "Dataset/GameState.hpp"




void push_state(std::vector<uint8_t>& file_buffer, VersusGame::State& game, game_state_datum& p1, game_state_datum& p2);

game_state_datum make_data(const Game& game, const Move& move, int& damage_sent);


enum class State {
    PLAYING,
    SETUP,
    GAME_OVER,
};

int main(int argc, char* argv[]) {
    // the args should look like this: ./a.out <bot1> <bot2> <pps>

    std::span<char*> args(argv, argc);
    // push to vector
    std::vector<std::string> vargs;
    for (auto& arg : args) {
        vargs.push_back(arg);
    }

#include "cli_main.hpp"


    // check if the args are correct
    if (vargs.size() < 4) {
        std::cerr << "Usage: " << std::filesystem::path(vargs[0]).filename() << " <bot1> <bot2> <pps> <optional:save_path>" << std::endl;
        return 1;
    }

    // pieces per second that the bots will play at
    float pps = 0.0f;
    // updates per second
    constexpr int UPS = 1000;
    const float dt = 1.0f / UPS;
    int frameCount = 0;
    float accumulator = 0.0f;
    double now = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1e9;

    try {
        pps = std::stof(vargs[3]);
    } catch (const std::exception&) {
        std::cerr << "pps must be a number" << std::endl;
        return 1;
    }

    // player interfaces for the bots
    Bot player_1;

    // start the bot
    player_1.start(vargs[1].c_str());

    Bot player_2;

    // start the bot
    player_2.start(vargs[2].c_str());

    // create the game
    VersusGame game;
    std::string binary_path = vargs.size() > 4 ? vargs[4] : "data.bin";
    std::ofstream file(binary_path, std::ios::binary | std::ios_base::app);
    std::vector<u8> file_buffer;

    State game_state = State::SETUP;

    // recorded stats
    std::array<int, 2> num_wins = {0, 0};
    int num_games = 0;
    int num_draws = 0;

    auto restart_bot_game = [](Bot& bot, Game& game) {
        std::vector<PieceType> tbp_queue(Game::queue_size + 1);
        tbp_queue[0] = game.current_piece.type;
        for (size_t i = 0; i < Game::queue_size; i++) {
            tbp_queue[i + 1] = game.queue[i];
        }
        bot.TBP_start(game.board, tbp_queue, game.hold, game.stats.b2b != 0, game.stats.combo);
    };
    
    // add boolean for changing the value while debugging
    bool running = true;
    while (running) {
        switch (game_state) {
            case State::PLAYING: {
                if (game.game_over) {
                    game_state = State::GAME_OVER;

                    Move empty_move;
                    int empty_attack = 0;
                    game_state_datum p1(make_data(game.p1_game, empty_move, empty_attack));
                    game_state_datum p2(make_data(game.p2_game, empty_move, empty_attack));

                    push_state(file_buffer, game.state, p1, p2);

                    break;
                }

                // if need to move then ask the bots for moves
                if (frameCount >= UPS / pps) {
                    bool no_moves_returned = false;
                    Piece suggestion_1 = PieceType::Empty;
                    Piece suggestion_2 = PieceType::Empty;

                    player_1.TBP_suggest();
                    auto p1_suggestions = player_1.TBP_suggestion();

                    if (p1_suggestions.empty()) {
                        // this is a band-aid patch 
                        // the bot may have different death rules than what we have in our implementation which causes no moves to be returned
                        game_state = State::SETUP;
                        break;
                    }

                    suggestion_1 = p1_suggestions.back();


                    player_2.TBP_suggest();
                    auto p2_suggestions = player_2.TBP_suggestion();
                    if (p2_suggestions.empty()) {
                        game_state = State::SETUP;
                        break;
                    }

                    suggestion_2 = p2_suggestions.back();


                    game.p1_move.null_move = false;
                    game.p1_move.piece = suggestion_1;

                    bool p1_first_hold = false;
                    if (!game.p1_game.hold && suggestion_1.type != game.p1_game.current_piece.type) {
                        p1_first_hold = true;
                    }

                    bool p2_first_hold = false;
                    if (!game.p2_game.hold && suggestion_2.type != game.p2_game.current_piece.type) {
                        p2_first_hold = true;
                    }

                    game.p2_move.null_move = false;
                    game.p2_move.piece = suggestion_2;


                    game_state_datum p1(make_data(game.p1_game, game.p1_move, game.p1_damage_sent));
                    game_state_datum p2(make_data(game.p2_game, game.p2_move, game.p2_damage_sent));
                    VersusGame::State s = VersusGame::State::PLAYING;

                    game.play_moves();

                    p1.attack = game.p1_damage_sent;
                    p1.damage_received = game.p2_damage_sent;
                    p1.spun = game.p1_spun;

                    p2.attack = game.p2_damage_sent;
                    p2.damage_received = game.p1_damage_sent;
                    p2.spun = game.p2_spun;

                    // save the data to file buffer
                    push_state(file_buffer, s, p1, p2);

                    bool p2_play = false;
                    if (game.p2_accepts_garbage) {
                        restart_bot_game(player_2, game.p2_game);
                    } else {
                        if (p2_first_hold) {
                            player_2.TBP_new_piece(game.p2_game.queue[3]);
                        }
                        player_2.TBP_new_piece(game.p2_game.queue.back());
                        p2_play = true;
                    }

                    bool p1_play = false;
                    if (game.p1_accepts_garbage) {
                        restart_bot_game(player_1, game.p1_game);
                    } else {
                        if (p1_first_hold)
                            player_1.TBP_new_piece(game.p1_game.queue[3]);
                        player_1.TBP_new_piece(game.p1_game.queue.back());

                        p1_play = true;
                    }

                    if (p2_play)
                        player_2.TBP_play(suggestion_2);

                    if (p1_play)
                        player_1.TBP_play(suggestion_1);

                    frameCount = 0;
                }

                // find out if its time to update the framecount
                double new_now = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1e9;
                double frame_time = new_now - now;
                now = new_now;
                accumulator += frame_time;
                while (accumulator >= dt) {
                    accumulator -= dt;
                    frameCount++;
                }
            } break;

            case State::SETUP: {
                game = VersusGame();

                restart_bot_game(player_2, game.p2_game);

                restart_bot_game(player_1, game.p1_game);
                frameCount = 0;

                game_state = State::PLAYING;
            } break;

            case State::GAME_OVER: {
                // save stats about the game here
                file.write((char*)file_buffer.data(), file_buffer.size());
                file_buffer.clear();
                file.flush();

                if (game.state == VersusGame::State::P1_WIN)
                    num_wins[0]++;
                else if (game.state == VersusGame::State::P2_WIN)
                    num_wins[1]++;
                else if (game.state == VersusGame::State::DRAW)
                    num_draws++;

                num_games++;
                // clear console
                std::cout << "\033[2J\033[1;1H";
                std::cout << "Player 1 wins: " << num_wins[0] << "\nPlayer 2 wins: " << num_wins[1] << "\nDraws: " << num_draws << "\nTotal games: " << num_games << std::endl;

                game_state = State::SETUP;
            } break;

        }  // end switch
    }

    std::cout << "Ended" << std::endl;
    return 0;
}

game_state_datum make_data(const Game& game, const Move& move, int& damage_sent)
{
    game_state_datum d{};
    d.b = game.board;

    d.p_type = (u8)game.current_piece.type;

    d.m_type = (u8)move.piece.type;
    d.m_rot = move.piece.rotation;
    d.m_x = (u8)move.piece.position.x;
    d.m_y = (u8)move.piece.position.y;

    d.meter = (u8)game.garbage_meter;

    d.queue[0] = (u8)game.queue[0];
    d.queue[1] = (u8)game.queue[1];
    d.queue[2] = (u8)game.queue[2];
    d.queue[3] = (u8)game.queue[3];
    d.queue[4] = (u8)game.queue[4];

    d.hold = game.hold.has_value() ? (u8)game.hold.value().type : 7;
    return d;
}

void push_state(std::vector<uint8_t>& file_buffer, VersusGame::State& state, game_state_datum& p1, game_state_datum& p2)
{

    /*
    file_buffer.append_range(std::span((u8*)&game.state, sizeof(VersusGame::State))); // one byte
    file_buffer.append_range(std::span((u8*)&p1, sizeof(data))); // 52 bytes
    file_buffer.append_range(std::span((u8*)&p2, sizeof(data))); // 52 bytes
    */
    // give me an alternative to append_range
    file_buffer.insert(file_buffer.end(), (u8*)&state, (u8*)&state + sizeof(VersusGame::State));
    file_buffer.insert(file_buffer.end(), (u8*)&p1, (u8*)&p1 + sizeof(game_state_datum));
    file_buffer.insert(file_buffer.end(), (u8*)&p2, (u8*)&p2 + sizeof(game_state_datum));
}
