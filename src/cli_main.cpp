#include <iostream>

#include "Bot.hpp"
#include "VersusGame.hpp"

enum class State {
    PLAYING,
    SETUP,
    GAME_OVER,
};


int main(int argc, char**argv) {
	// the args should look like this: ./a.out <bot1> <bot2> <pps>

	std::span<char*> args(argv, argc);
    // push to vector
	std::vector<std::string> vargs;
	for (auto& arg : args) {
		vargs.push_back(arg);
    }

	// check if the args are correct
    if (vargs.size() != 4) {
		std::cerr << "Usage: " << std::filesystem::path(vargs[0]).filename() << " <bot1> <bot2> <pps>" << std::endl;
		return 1;
    }

    // pieces per second that the bots will play at
    float pps = 0.0f;
    try
    {
        pps = std::stof(vargs[3]);

    }
    catch (const std::exception&)
    {
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
    State game_state = State::SETUP;
    // the frame count for the bots to determine when they should move
    int frameCount = 0;
    // recorded stats
    std::array<int, 2> num_wins = { 0, 0 };
    int num_games = 0;
    int num_draws = 0;
	constexpr int UPS = 100;


    auto restart_bot_game = [](Bot& bot, Game& game) {
        std::vector<PieceType> tbp_queue(Game::queue_size + 1);
        tbp_queue[0] = game.current_piece.type;
        for (size_t i = 0; i < Game::queue_size; i++) {
            tbp_queue[i + 1] = game.queue[i];
        }
        bot.TBP_start(game.board, tbp_queue, game.hold, game.stats.b2b != 0, game.stats.combo);
        };
    switch (game_state) {

    case State::PLAYING: {
        if (game.game_over) {
            game_state = State::GAME_OVER;
        }

        // if need to move then ask the bots for moves
        if (frameCount >= UPS / pps) {
            player_1.TBP_suggest();
            Piece suggestion_1 = player_1.TBP_suggestion()[0];

            player_2.TBP_suggest();
            Piece suggestion_2 = player_2.TBP_suggestion()[0];

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

            game.play_moves();

            bool p2_play = false;
            if (game.p2_accepts_garbage)
            {
                restart_bot_game(player_2, game.p2_game);
            }
            else {
                if (p2_first_hold)
                    player_2.TBP_new_piece(game.p2_game.queue[3]);
                player_2.TBP_new_piece(game.p2_game.queue.back());
                p2_play = true;
            }




            bool p1_play = false;
            if (game.p1_accepts_garbage)
            {
                restart_bot_game(player_1, game.p1_game);
            }
            else {
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

        frameCount++;
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
        if (game.state == VersusGame::State::P1_WIN)
        {
            num_wins[0]++;
        }
        else if (game.state == VersusGame::State::P2_WIN)
        {
            num_wins[1]++;
        }
        else if (game.state == VersusGame::State::DRAW)
        {
            num_draws++;
        }
        num_games++;
        std::cout.setstate(std::ios_base::goodbit);
        std::cout.clear();
        // clear console
        std::cout << "\033[2J\033[1;1H";
        std::cout <<
            "Player 1 wins: " << num_wins[0] <<
            "\nPlayer 2 wins: " << num_wins[1] <<
            "\nDraws: " << num_draws <<
            "\nTotal games: " << num_games << std::endl;
        std::cout.setstate(std::ios_base::failbit);


        game_state = State::SETUP;
    } break;

    } // end switch

	return 0;
}