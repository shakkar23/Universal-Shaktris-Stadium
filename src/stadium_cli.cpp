#include <array>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <thread>
#include <cstdlib>
#include <expected>
#include <bitset>

#include "Bot.hpp"
#include "Dataset/GameState.hpp"
#include "VersusGame.hpp"
#include "Util/argparse.hpp"

#include "sqlite3.h"

#include <algorithm>

using u8 = uint8_t;


struct sqlite_row {

	// board
	std::array<u8, 10 * 20> b;

	// current piece type
	u8 p_type;

	// move
	u8 m_type;
	u8 m_rot;
	u8 m_x;
	u8 m_y;

	// extra data
	u8 meter;
	u8 attack;
	u8 damage_received;
	u8 spun;
	u8 queue[5];
	u8 hold;
	int combo;
	int b2b;
};

void push_state(sqlite3* db, VersusGame::State& state, sqlite_row& p1, sqlite_row& p2, sqlite3_int64 game_uuid, int move_index);

static sqlite_row prepare_row(const Game& game, const Move& move, int damage_sent, int meter);

struct game_state {
	VersusGame::State state;
	sqlite_row p1;
	sqlite_row p2;
	sqlite3_int64 game_uuid;
	int move_index;
};

enum class State {
	PLAYING,
	SETUP,
	GAME_OVER,
};

sqlite3* database{ nullptr };
sqlite3_stmt* stmt = nullptr;

bool init_stmt(sqlite3* db) {
	const char* stmt_str = "INSERT INTO Data (game_id, move_index, state,"
		"p1_board,p1_current_piece,p1_move_piece_type,p1_move_piece_rot,p1_move_piece_x,p1_move_piece_y,p1_meter,p1_attack,p1_damage_received,p1_spun,p1_queue_0,p1_queue_1,p1_queue_2,p1_queue_3,p1_queue_4,p1_hold,p1_combo, p1_b2b,"
		"p2_board,p2_current_piece,p2_move_piece_type,p2_move_piece_rot,p2_move_piece_x,p2_move_piece_y,p2_meter,p2_attack,p2_damage_received,p2_spun,p2_queue_0,p2_queue_1,p2_queue_2,p2_queue_3,p2_queue_4,p2_hold,p2_combo, p2_b2b"
		") VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";

	int ret = sqlite3_prepare(db, stmt_str, -1, &stmt, nullptr);
	if(ret != SQLITE_OK) {
		fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
		throw std::runtime_error("");
		return false;
	}

	return true;
};

bool create_table(sqlite3* db) {
	char* err_msg = 0;

	// We use a multi-line string for clarity. 
	// In C, adjacent string literals are automatically concatenated.
	const char* sql =
		"CREATE TABLE IF NOT EXISTS Data ("
		"game_id INTEGER NOT NULL, "
		"move_index INTEGER NOT NULL, "
		"state TEXT NOT NULL, "
		"p1_board BLOB NOT NULL, p1_current_piece TEXT NOT NULL, p1_move_piece_type TEXT NOT NULL, "
		"p1_move_piece_rot INTEGER NOT NULL, p1_move_piece_x INTEGER NOT NULL, p1_move_piece_y INTEGER NOT NULL, "
		"p1_meter INTEGER NOT NULL, p1_attack INTEGER NOT NULL, p1_damage_received INTEGER NOT NULL, "
		"p1_spun INTEGER NOT NULL, "
		"p1_queue_0 TEXT NOT NULL, p1_queue_1 TEXT NOT NULL, p1_queue_2 TEXT NOT NULL, p1_queue_3 TEXT NOT NULL, p1_queue_4 TEXT NOT NULL, "
		"p1_hold TEXT NOT NULL, " 
		"p1_combo INTEGER NOT NULL, "
		"p1_b2b INTEGER NOT NULL, "

		"p2_board BLOB NOT NULL, p2_current_piece TEXT NOT NULL, p2_move_piece_type TEXT NOT NULL, "
		"p2_move_piece_rot INTEGER NOT NULL, p2_move_piece_x INTEGER NOT NULL, p2_move_piece_y INTEGER NOT NULL, "
		"p2_meter INTEGER NOT NULL, p2_attack INTEGER NOT NULL, p2_damage_received INTEGER NOT NULL, "
		"p2_spun INTEGER NOT NULL, "
		"p2_queue_0 TEXT NOT NULL, p2_queue_1 TEXT NOT NULL, p2_queue_2 TEXT NOT NULL, p2_queue_3 TEXT NOT NULL, p2_queue_4 TEXT NOT NULL, "
		"p2_hold TEXT NOT NULL, "
		"p2_combo INTEGER NOT NULL, "
		"p2_b2b INTEGER NOT NULL, "
		"PRIMARY KEY(game_id, move_index)"
		");";

	// sqlite3_exec is the best choice for simple CREATE/DROP/DELETE commands
	int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);

	if(rc != SQLITE_OK) {
		fprintf(stderr, "SQL error (Create Table): %s\n", err_msg);
		// We must free the error message allocated by sqlite3_exec
		sqlite3_free(err_msg);
		throw std::runtime_error("");
		return false;
	}
	return true;
}

int get_next_game_id(sqlite3* db) {
	// Static statement persists for the lifetime of the program
	static sqlite3_stmt* stmt = nullptr;

	// Initialize the statement only once
	if(stmt == nullptr) {
		const char* sql = "SELECT IFNULL(MAX(game_id), 0) + 1 FROM Data;";
		int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

		if(rc != SQLITE_OK) {
			auto err = std::string("Failed to prepare statement: ") + sqlite3_errmsg(db);
			std::cerr << err << std::endl;
			throw std::runtime_error(err);
		}
	}

	int next_id = 1; // Default if table is empty
	int rc = sqlite3_step(stmt);

	if(rc == SQLITE_ROW) {
		next_id = sqlite3_column_int(stmt, 0);
	} else {
		auto err = std::string("Execution failed: ") + sqlite3_errmsg(db);
		std::cerr << "Execution failed: " << sqlite3_errmsg(db) << std::endl;
		throw std::runtime_error(err);
	}

	// Reset the statement so it can be used again next time
	sqlite3_reset(stmt);
	return next_id;
}

void sigint_handler(int signal) {
	sqlite3_finalize(stmt);
	sqlite3_close(database);
	std::abort();
}

constexpr auto SETTINGS_PATH = "stadium_cli_settings.json";
void init_settings() {
	auto rjson = R"({
	"num_games":100,
	"db_path":"database.db",
	"min_nodes":2000,
	"pps":2
})";
	auto exists = std::filesystem::exists(SETTINGS_PATH);
	if(exists)
		return;

	std::fstream f(SETTINGS_PATH, std::ios::out);
	f << rjson;
}
nlohmann::json get_settings() {
	std::fstream f(SETTINGS_PATH, std::ios::in);
	return nlohmann::json::parse((std::stringstream() << f.rdbuf()).str());
}

int main(int argc, char** argv) {
	argparse::ArgumentParser program("stadium_cli");
	program.add_argument("-game_type").choices("TETRIO1", "PPT");
	program.add_argument("bots").remaining().nargs(2);
	std::string bot1, bot2;
	game_type gt;
	try {
		program.parse_args(argc, argv);

		auto bots = program.get<std::vector<std::string>>("bots");
		bot1 = bots.at(0);
		bot2 = bots.at(1);
		auto gt_param = program.get("-game_type");
		if(gt_param == "PPT") {
			gt = game_type::PPT;
		} else {
			gt = game_type::Tetrio;
		}
		
	} catch(std::exception& err) {
		std::cerr << "failed to parse arguments: " << err.what() << std::endl;
		return 1;
	}
	std::cout << bot1 << bot2 << std::endl;
	init_settings();
	auto settings = get_settings();

	// if(argc < 2)
	// 	vargs = { "lmao", "/root/cold-clear-data/target/release/cc-tbp", "/root/cold-clear-data/target/release/cc-tbp" };

	// pieces per second that the bots will play at
	float pps = 0.0f;
	float seconds_per_piece = 0.0f;
	double now = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1e9;

	try {
		pps = settings["pps"].get<double>();
		seconds_per_piece = 1.0f / pps;
	} catch(const std::exception&) {
		std::cerr << "pps must be a number" << std::endl;
		return 1;
	}

	// player interfaces for the bots
	Bot player_1;
	// start the bot
	player_1.start(bot1.c_str(), settings["min_nodes"].get<long long>());
	player_1.name += '1';

	Bot player_2;
	
	// start the bot
	player_2.start(bot2.c_str(), settings["min_nodes"].get<long long>());
	player_2.name += '2';

	// create the game
	VersusGame game(game_type::PPT);
	std::string binary_path = settings["db_path"].get<std::string>();

	std::vector<game_state> game_states;
	int sql_ret = sqlite3_open(binary_path.c_str(), &database);
	
	if(sql_ret != SQLITE_OK) {
		std::cout << "couldnt open the database: " << binary_path << ", " << sqlite3_errmsg(database) << std::endl;
		sqlite3_close(database);
		return 1;
	}

	if(!create_table(database)) {
		sqlite3_finalize(stmt);
		sqlite3_close(database);
		return 1;
	}

	if(!init_stmt(database)) {
		std::cout << "couldnt prepare statement: " << sqlite3_errmsg(database) << std::endl;
		sqlite3_finalize(stmt);
		sqlite3_close(database);
		return 1;
	}
	std::signal(SIGINT, sigint_handler);

	State game_state = State::SETUP;
	int game_uuid = get_next_game_id(database);
	int move_index = 0;

	// recorded stats
	std::array<int, 2> num_wins = { 0, 0 };
	long long num_games = 0;
	int num_draws = 0;

	auto restart_bot_game = [](Bot& bot, Game& game, Game& opp) {
		std::vector<PieceType> tbp_queue(Game::queue_size + 1);
		tbp_queue[0] = game.current_piece.type;
		for(size_t i = 0; i < Game::queue_size; i++) {
			tbp_queue[i + 1] = game.queue[i];
		}
		bot.TBP_start(game, opp);
	};

	// add boolean for changing the value while debugging
	const long long total_num_games = settings["num_games"].get<long long>();
	while(num_games < total_num_games) {
		switch(game_state) {
			case State::PLAYING:
			{
				// if need to move then ask the bots for moves
				bool no_moves_returned = false;
				
				auto handle_suggest = [](Bot&bot) -> std::expected<Piece,bool>{
					bot.TBP_suggest();
					auto suggestion = bot.TBP_suggestion();

					if(suggestion.empty()) {
						// this is a band-aid patch 
						// the bot may have different death rules than what we have in our implementation which causes no moves to be returned
						return std::unexpected(true);
					}
					return suggestion.back();
				};
				enum class piece_invalid_err {
					no_err,
					out_of_bounds,
					floating,
					wrong_piece,
					wrong_piece2,
				};
				auto check_piece_valid = [](Game&game, Piece p) -> piece_invalid_err {
					bool collides = game.collides(game.board, p);
					// inside board bad
					if(collides)
						return piece_invalid_err::out_of_bounds;

					Piece lower_piece = p;
					lower_piece.position.y -= 1;
					bool in_bounds = true;
					for(auto mino:lower_piece.minos) {
						// if all are in bounds
						bool good = true;
						
						auto x = mino.x + lower_piece.position.x;
						auto y = mino.y + lower_piece.position.y;

						good &= x >= 0;
						good &= x < game.board.width;
						good &= y >= 0;
						good &= y < game.board.height;
						
						in_bounds &= good;
					}
					// floating bad
					if(in_bounds and not game.collides(game.board, lower_piece))
						return piece_invalid_err::floating;

					if(not game.hold.has_value() and p.type != game.queue.front() and p.type != game.current_piece.type)
						return piece_invalid_err::wrong_piece;
					if(	game.hold.has_value() and 
						game.hold.value().type != p.type and 
						game.current_piece.type != p.type)
						return piece_invalid_err::wrong_piece2;
					return piece_invalid_err::no_err;
				};
				
				// need bool so the bot 2 can flush its suggestion before we restart
				// if we dont do this it causes a bug where we get a piece placement meant for the previous game on the current one
				bool failed_suggest = false;
				const auto suggestion_1_expectation = handle_suggest(player_1);
				if(not suggestion_1_expectation.has_value()) {
					failed_suggest = true;
				}
				const Piece suggestion_1 = suggestion_1_expectation.value();
				
				auto err1 = check_piece_valid(game.p1_game, suggestion_1);

				if(err1 != piece_invalid_err::no_err) {
					for(auto mino:suggestion_1.minos) {
						std::cout << game.p1_game.board.get(mino.x + suggestion_1.position.x,mino.y + suggestion_1.position.y);
					}
					std::cout << std::endl;
					for(auto col : game.p1_game.board.board) {
						std::cout << std::bitset<32>(col) << std::endl;
					}
					//const auto suggestion_1_expectation2 = handle_suggest(player_1);
					throw std::runtime_error("player 1 gave invalid piece");
				}
				
				const auto suggestion_2_expectation = handle_suggest(player_2);
				if(not suggestion_2_expectation.has_value()) {
					failed_suggest = true;
				}
				const Piece suggestion_2 = suggestion_2_expectation.value();

				if(failed_suggest) {
					game_state = State::SETUP;
					break;
				}
				
				auto err2 = check_piece_valid(game.p2_game, suggestion_2);
				if(err2 != piece_invalid_err::no_err) {
					for(auto mino:suggestion_2.minos) {
						std::cout << game.p2_game.board.get(mino.x + suggestion_2.position.x,mino.y + suggestion_2.position.y);
					}
					std::cout << std::endl;
					for(auto col : game.p2_game.board.board) {
						std::cout << std::bitset<32>(col) << std::endl;
					}
					//const auto suggestion_2_expectation2 = handle_suggest(player_2);
					throw std::runtime_error("player 2 gave invalid piece");
				}

				auto is_first_hold = [](Game& game, const Piece&suggestion) {
					bool first_hold = false;
					if(!game.hold && suggestion.type != game.current_piece.type) {
						first_hold = true;
					}
					return first_hold;
				};

				bool p1_first_hold = is_first_hold(game.p1_game, suggestion_1);
				game.p1_move = Move(suggestion_1);

				bool p2_first_hold = is_first_hold(game.p2_game, suggestion_2);
				game.p2_move = Move(suggestion_2);

				sqlite_row p1(prepare_row(game.p1_game, game.p1_move, game.p1_damage_sent, game.p1_meter));
				sqlite_row p2(prepare_row(game.p2_game, game.p2_move, game.p2_damage_sent, game.p2_meter));
				VersusGame::State s = VersusGame::State::PLAYING;

				game.play_moves();
				
				if(game.game_over) {
					game_state = State::GAME_OVER;

					Move empty_move;
					sqlite_row p1(prepare_row(game.p1_game, empty_move, 0, game.p1_meter));
					sqlite_row p2(prepare_row(game.p2_game, empty_move, 0, game.p2_meter));
					game_states.push_back({ game.state, p1, p2, game_uuid, move_index });
					break;
				}

				// data stuff
				p1.attack = game.p1_damage_sent;
				p1.damage_received = game.p2_damage_sent;
				p1.spun = game.p1_spun;

				p2.attack = game.p2_damage_sent;
				p2.damage_received = game.p1_damage_sent;
				p2.spun = game.p2_spun;

				// save the data to file buffer
				game_states.push_back({ s, p1, p2, game_uuid, move_index });
				move_index++;

				
				if(game.p1_accepts_garbage) {
					restart_bot_game(player_1, game.p1_game, game.p2_game);
				} else {
					if(p1_first_hold)
						player_1.TBP_new_piece(game.p1_game.queue[3]);
					player_1.TBP_new_piece(game.p1_game.queue.back());
					player_1.TBP_play(game.p1_game, game.p2_game, suggestion_1);
				}

				if(game.p2_accepts_garbage) {
					restart_bot_game(player_2, game.p2_game, game.p1_game);
				} else {
					if(p2_first_hold)
						player_2.TBP_new_piece(game.p2_game.queue[3]);
					player_2.TBP_new_piece(game.p2_game.queue.back());
					player_2.TBP_play(game.p2_game, game.p1_game, suggestion_2);
				}

				// find out if its time to update the framecount
				if(not player_1.should_skip_suggest() or not player_2.should_skip_suggest())
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
				for(auto& state : game_states) {
					push_state(database, state.state, state.p1, state.p2, state.game_uuid, state.move_index);
				}
				game_uuid = get_next_game_id(database);
				move_index = 0;
				if(std::ranges::count_if(game_states, [](const auto& state) {return state.state != VersusGame::State::PLAYING; }) != 1) {
					throw std::runtime_error("uh oh");
				}
				game_states.clear(); 
				game_state = State::SETUP;
			} break;

		}  // end switch
	}

	player_1.stop();
	player_2.stop();
	
	std::cout << "Ended" << std::endl;
	sqlite3_finalize(stmt);
	sqlite3_close(database);

	return 0;
}

sqlite_row prepare_row(const Game& game, const Move& move, int damage_sent, int meter) {
	sqlite_row d{};

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

	d.meter = (u8)meter;

	d.queue[0] = (u8)game.queue[0];
	d.queue[1] = (u8)game.queue[1];
	d.queue[2] = (u8)game.queue[2];
	d.queue[3] = (u8)game.queue[3];
	d.queue[4] = (u8)game.queue[4];

	d.hold = game.hold.has_value() ? (u8)game.hold.value().type : 7;
	d.combo = game.get_combo();
	d.b2b = game.get_b2b();
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
	} .at(type);
}

void push_state(sqlite3* db, VersusGame::State& state, sqlite_row& p1, sqlite_row& p2, sqlite3_int64 game_uuid, int move_index) {

	/*
	file_buffer.append_range(std::span((u8*)&game.state, sizeof(VersusGame::State))); // one byte
	file_buffer.append_range(std::span((u8*)&p1, sizeof(data))); // 52 bytes
	file_buffer.append_range(std::span((u8*)&p2, sizeof(data))); // 52 bytes
	//
	file_buffer.insert(file_buffer.end(), (u8*)&state, (u8*)&state + sizeof(VersusGame::State));
	file_buffer.insert(file_buffer.end(), (u8*)&p1, (u8*)&p1 + sizeof(sqlite_row));
	file_buffer.insert(file_buffer.end(), (u8*)&p2, (u8*)&p2 + sizeof(sqlite_row));
	*/
	int index = 1;
	int rv;

	sqlite3_bind_int64(stmt, index, game_uuid);
	index++;
	sqlite3_bind_int(stmt, index, move_index);
	index++;
	sqlite3_bind_text(stmt, index, std::array{ "PLAYING","P1_WIN","P2_WIN","DRAW" } .at((size_t)state) , -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_blob(stmt, index, p1.b.data(), 200, SQLITE_TRANSIENT);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.p_type), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.m_type), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_int(stmt, index, (int)p1.m_rot);
	index++;
	sqlite3_bind_int(stmt, index, (int)p1.m_x);
	index++;
	sqlite3_bind_int(stmt, index, (int)p1.m_y);
	index++;
	sqlite3_bind_int(stmt, index, (int)p1.meter);
	index++;
	sqlite3_bind_int(stmt, index, (int)p1.attack);
	index++;
	sqlite3_bind_int(stmt, index, (int)p1.damage_received);
	index++;
	sqlite3_bind_int(stmt, index, (int)p1.spun);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.queue[0]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.queue[1]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.queue[2]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.queue[3]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.queue[4]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p1.hold), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_int(stmt, index, p1.combo);
	index++;
	sqlite3_bind_int(stmt, index, p1.b2b);
	index++;


	sqlite3_bind_blob(stmt, index, p2.b.data(), 200, SQLITE_TRANSIENT);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.p_type), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.m_type), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_int(stmt, index, (int)p2.m_rot);
	index++;
	sqlite3_bind_int(stmt, index, (int)p2.m_x);
	index++;
	sqlite3_bind_int(stmt, index, (int)p2.m_y);
	index++;
	sqlite3_bind_int(stmt, index, (int)p2.meter);
	index++;
	sqlite3_bind_int(stmt, index, (int)p2.attack);
	index++;
	sqlite3_bind_int(stmt, index, (int)p2.damage_received);
	index++;
	sqlite3_bind_int(stmt, index, (int)p2.spun);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.queue[0]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.queue[1]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.queue[2]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.queue[3]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.queue[4]), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_text(stmt, index, type_to_str(p2.hold), -1, SQLITE_STATIC);
	index++;
	sqlite3_bind_int(stmt, index, p2.combo);
	index++;
	sqlite3_bind_int(stmt, index, p2.b2b);
	index++;

	rv = sqlite3_step(stmt);

	if(rv != SQLITE_DONE) {
		std::cout << "insert error: " << sqlite3_errmsg(database) << std::endl;
		int offset = sqlite3_error_offset(db);
		std::cout << "Error at character: " << offset << std::endl << sqlite3_errmsg(db) << std::endl;
		std::cout << index << " " << game_uuid <<" "<< rv<<" "<<move_index << std::endl;
		throw std::runtime_error(std::string("insert_error: ") + sqlite3_errmsg(database) + '\n' + "Error at character: " + std::to_string(offset) + sqlite3_errmsg(db));
	}

	sqlite3_reset(stmt);
}
