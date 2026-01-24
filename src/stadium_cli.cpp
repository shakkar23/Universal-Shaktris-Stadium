#include <fstream>
#include <iostream>
#include <thread>
#include <span>
#include <filesystem>
#include <chrono>
#include <array>
#include <cstdio>

#include "Bot.hpp"
#include "VersusGame.hpp"
#include "Dataset/GameState.hpp"

#include "sqlite3.h"


void push_state(sqlite3* db, VersusGame::State& game, game_state_datum& p1, game_state_datum& p2);

game_state_datum make_data(const Game& game, const Move& move, int damage_sent);


enum class State {
	PLAYING,
	SETUP,
	GAME_OVER,
};

sqlite3_stmt* stmt = nullptr;
bool init_stmt(sqlite3* db) {
	const char* stmt_str = "INSERT INTO Data (state,p1_board,p1_current_piece,p1_move_piece_type,p1_move_piece_rot,p1_move_piece_x,p1_move_piece_y,p1_meter,p1_attack,p1_damage_received,p1_spun,p1_queue_0,p1_queue_1,p1_queue_2,p1_queue_3,p1_queue_4,p1_hold,p2_board,p2_current_piece,p2_move_piece_type,p2_move_piece_rot,p2_move_piece_x,p2_move_piece_y,p2_meter,p2_attack,p2_damage_received,p2_spun,p2_queue_0,p2_queue_1,p2_queue_2,p2_queue_3,p2_queue_4,p2_hold) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
	
	int ret = sqlite3_prepare(db, stmt_str, -1, &stmt, nullptr);
	if(ret != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		return false;
	}
	
	return true;
};

bool create_table(sqlite3* db) {
	char* err_msg = 0;

	// We use a multi-line string for clarity. 
	// In C, adjacent string literals are automatically concatenated.
	const char* sql =
		"CREATE TABLE IF NOT EXISTS GameData ("
		"state TEXT, "
		"p1_board INTEGER, p1_current_piece INTEGER, p1_move_piece_type TEXT, "
		"p1_move_piece_rot INTEGER, p1_move_piece_x INTEGER, p1_move_piece_y INTEGER, "
		"p1_meter INTEGER, p1_attack INTEGER, p1_damage_received INTEGER, "
		"p1_spun INTEGER, "
		"p1_queue_0 TEXT, p1_queue_1 TEXT, p1_queue_2 TEXT, p1_queue_3 TEXT, p1_queue_4 TEXT, "
		"p1_hold INTEGER, "
		"p2_board INTEGER, p2_current_piece INTEGER, p2_move_piece_type TEXT, "
		"p2_move_piece_rot INTEGER, p2_move_piece_x INTEGER, p2_move_piece_y INTEGER, "
		"p2_meter INTEGER, p2_attack INTEGER, p2_damage_received INTEGER, "
		"p2_spun INTEGER, "
		"p2_queue_0 TEXT, p2_queue_1 TEXT, p2_queue_2 TEXT, p2_queue_3 TEXT, p2_queue_4 TEXT, "
		"p2_hold INTEGER"
		");";

	// sqlite3_exec is the best choice for simple CREATE/DROP/DELETE commands
	int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "SQL error (Create Table): %s\n", err_msg);
		// We must free the error message allocated by sqlite3_exec
		sqlite3_free(err_msg);
		return false;
	}
	return true;
}

int main(int argc, char* argv[]) {
	// the args should look like this: ./a.out <bot1> <bot2> <pps>

	std::span<char*> args(argv, argc);
	// push to vector
	std::vector<std::string> vargs;
	for(auto& arg : args) {
		vargs.push_back(arg);
	}

	//#include "cli_main.hpp"


		// check if the args are correct
	if(vargs.size() < 4) {
		std::cerr << "Usage: " << std::filesystem::path(vargs[0]).filename() << " <bot1> <bot2> <pps> <optional:save_path>" << std::endl;
		return 1;
	}

	// pieces per second that the bots will play at
	float pps = 0.0f;
	float seconds_per_piece = 0.0f;
	double now = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1e9;

	try {
		pps = std::stof(vargs[3]);
		seconds_per_piece = 1.0f / pps;
	} catch(const std::exception&) {
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
	std::string binary_path = vargs.size() > 4 ? vargs[4] : "database.db";
	
	sqlite3* database{nullptr};
	int sql_ret = sqlite3_open(binary_path.c_str(), &database);
	
	if(sql_ret != SQLITE_OK) {
		std::cout << "couldnt open the database: " << binary_path << ", " << sqlite3_errmsg(database) << std::endl;
		return 1;
	}
	
	if(!init_stmt(database)) {
		std::cout << "couldnt prepare statement: " << sqlite3_errmsg(database) << std::endl;
		return 1;
	}
	if(!create_table(database)) {
		return 1;
	}

	State game_state = State::SETUP;

	// recorded stats
	std::array<int, 2> num_wins = { 0, 0 };
	int num_games = 0;
	int num_draws = 0;

	auto restart_bot_game = [](Bot& bot, Game& game, Game& opp) {
		std::vector<PieceType> tbp_queue(Game::queue_size + 1);
		tbp_queue[0] = game.current_piece.type;
		for(size_t i = 0; i < Game::queue_size; i++) {
			tbp_queue[i + 1] = game.queue[i];
		}
		bot.TBP_start(opp, game.board, tbp_queue, game.hold, game.stats.b2b != 0, game.stats.combo);
	};

	// add boolean for changing the value while debugging
	bool running = true;
	while(running) {
		switch(game_state) {
			case State::PLAYING:
			{
				if(game.game_over) {
					game_state = State::GAME_OVER;

					Move empty_move;
					game_state_datum p1(make_data(game.p1_game, empty_move, 0));
					game_state_datum p2(make_data(game.p2_game, empty_move, 0));

					push_state(database, game.state, p1, p2);

					break;
				}

				// if need to move then ask the bots for moves
				bool no_moves_returned = false;
				Piece suggestion_1 = PieceType::Empty;
				Piece suggestion_2 = PieceType::Empty;

				player_1.TBP_suggest();
				auto p1_suggestions = player_1.TBP_suggestion();

				if(p1_suggestions.empty()) {
					// this is a band-aid patch 
					// the bot may have different death rules than what we have in our implementation which causes no moves to be returned
					game_state = State::SETUP;
					break;
				}

				suggestion_1 = p1_suggestions.back();


				player_2.TBP_suggest();
				auto p2_suggestions = player_2.TBP_suggestion();
				if(p2_suggestions.empty()) {
					game_state = State::SETUP;
					break;
				}

				suggestion_2 = p2_suggestions.back();


				game.p1_move.null_move = false;
				game.p1_move.piece = suggestion_1;

				bool p1_first_hold = false;
				if(!game.p1_game.hold && suggestion_1.type != game.p1_game.current_piece.type) {
					p1_first_hold = true;
				}

				bool p2_first_hold = false;
				if(!game.p2_game.hold && suggestion_2.type != game.p2_game.current_piece.type) {
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
				push_state(database, s, p1, p2);

				bool p2_play = false;
				if(game.p2_accepts_garbage) {
					restart_bot_game(player_2, game.p2_game, game.p1_game);
				} else {
					if(p2_first_hold) {
						player_2.TBP_new_piece(game.p2_game.queue[3]);
					}
					player_2.TBP_new_piece(game.p2_game.queue.back());
					p2_play = true;
				}

				bool p1_play = false;
				if(game.p1_accepts_garbage) {
					restart_bot_game(player_1, game.p1_game, game.p2_game);
				} else {
					if(p1_first_hold)
						player_1.TBP_new_piece(game.p1_game.queue[3]);
					player_1.TBP_new_piece(game.p1_game.queue.back());

					p1_play = true;
				}

				if(p2_play)
					player_2.TBP_play(game.p2_game, suggestion_2);

				if(p1_play)
					player_1.TBP_play(game.p2_game, suggestion_1);

				// find out if its time to update the framecount
				std::this_thread::sleep_until(std::chrono::steady_clock::now() + std::chrono::milliseconds(int(seconds_per_piece * 1000.0f)));
			} break;

			case State::SETUP:
			{
				game = VersusGame();

				restart_bot_game(player_2, game.p2_game, game.p1_game);

				restart_bot_game(player_1, game.p1_game, game.p2_game);

				game_state = State::PLAYING;
			} break;

			case State::GAME_OVER:
			{
				if(game.state == VersusGame::State::P1_WIN)
					num_wins[0]++;
				else if(game.state == VersusGame::State::P2_WIN)
					num_wins[1]++;
				else if(game.state == VersusGame::State::DRAW)
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
	sqlite3_finalize(stmt);
	sqlite3_close(database);
	
	return 0;
}

game_state_datum make_data(const Game& game, const Move& move, int damage_sent) {
	game_state_datum d{};

	std::array<u8, 10 * 20> board{};
	for(size_t x = 0; x < 10; x++)
		for(size_t y = 0; y < 20; y++) {
			board[x + y * 10] = game.board.get(x, y);
		}
	d.b = board;

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
const char* type_to_str(u8 type) {
	return std::array{
	"S",
	"Z",
	"J",
	"L",
	"T",
	"O",
	"I",
	"NULL"
	} [type] ;
}
void push_state(sqlite3* db, VersusGame::State& state, game_state_datum& p1, game_state_datum& p2) {
	
	/*
	file_buffer.append_range(std::span((u8*)&game.state, sizeof(VersusGame::State))); // one byte
	file_buffer.append_range(std::span((u8*)&p1, sizeof(data))); // 52 bytes
	file_buffer.append_range(std::span((u8*)&p2, sizeof(data))); // 52 bytes
	//
	file_buffer.insert(file_buffer.end(), (u8*)&state, (u8*)&state + sizeof(VersusGame::State));
	file_buffer.insert(file_buffer.end(), (u8*)&p1, (u8*)&p1 + sizeof(game_state_datum));
	file_buffer.insert(file_buffer.end(), (u8*)&p2, (u8*)&p2 + sizeof(game_state_datum));
	*/
			
	sqlite3_bind_text(stmt, 1, std::array{"PLAYING","P1_WIN","P2_WIN","DRAW"}[(size_t)state], -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_blob(stmt, 2, p1.b.data(), 200, SQLITE_TRANSIENT);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 3, type_to_str(p1.p_type), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 4, type_to_str(p1.m_type), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 5, (int)p1.m_rot);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 6, (int)p1.m_x);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 7, (int)p1.m_y);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 8, (int)p1.meter);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 9, (int)p1.attack);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 10, (int)p1.damage_received);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 11, (int)p1.spun);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 12, type_to_str(p1.queue[0]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 13, type_to_str(p1.queue[1]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 14, type_to_str(p1.queue[2]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 15, type_to_str(p1.queue[3]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 16, type_to_str(p1.queue[4]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 17, (int)p1.hold);
	sqlite3_step(stmt);



	sqlite3_bind_blob(stmt, 18, p2.b.data(), 200, SQLITE_TRANSIENT);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 19, type_to_str(p2.p_type), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 20, type_to_str(p2.m_type), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 21, (int)p2.m_rot);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 22, (int)p2.m_x);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 23, (int)p2.m_y);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 24, (int)p2.meter);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 25, (int)p2.attack);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 26, (int)p2.damage_received);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 27, (int)p2.spun);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 28, type_to_str(p2.queue[0]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 29, type_to_str(p2.queue[1]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 30, type_to_str(p2.queue[2]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 31, type_to_str(p2.queue[3]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_text(stmt, 32, type_to_str(p2.queue[4]), -1, SQLITE_STATIC);
	sqlite3_step(stmt);

	sqlite3_bind_int(stmt, 33, (int)p2.hold);
	sqlite3_step(stmt);

	sqlite3_reset(stmt);
}
