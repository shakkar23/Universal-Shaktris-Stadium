#pragma once
#include <optional>
#include <random>
#include <vector>

#include "Game.hpp"
#include "Move.hpp"
#include "Piece.hpp"
#include "rng.hpp"

enum Outcomes {
    P1_WIN,
    P2_WIN,
    DRAW,
    NONE
};

class VersusGame {
public:
 VersusGame() {
     p1_rng.rng = std::random_device()();
     p1_rng.makebag();

     p1_game.current_piece = p1_rng.GetPiece();
     for (auto& p : p1_game.queue) {
         p = p1_rng.GetPiece();
     }

     p2_rng.rng = std::random_device()();
     p2_rng.makebag();

     p2_game.current_piece = p2_rng.GetPiece();
     for (auto& p : p2_game.queue) {
         p = p2_rng.GetPiece();
     }
 }
    Game p1_game;
    Game p2_game;

    Move p1_move = Move(Piece(PieceType::Empty), false);
    Move p2_move = Move(Piece(PieceType::Empty), false);

	RNG p1_rng;
	RNG p2_rng;

    double p1_atk = 0;
    double p2_atk = 0;

    int p1_meter = 0;
    int p2_meter = 0;

    int turn = 0;
    bool game_over = false;

    // get attack per piece
    double inline get_app(int id) const {
        return id == 0 ? p1_atk / turn : p2_atk / turn;
    }

    double inline get_b2b(int id) const {
        return id == 0 ? p1_game.stats.b2b : p2_game.stats.b2b;
    }

    const inline Game& get_game(int id) const {
        return id == 0 ? p1_game : p2_game;
    }

    void set_move(int id, Move move)
	{
		if (id == 0) {
			p1_move = move;
		}
		else {
			p2_move = move;
		}
	}


	void play_moves()
	{
		// game over conditions

		if (game_over) {
			return;
		}

		if (p1_game.collides(p1_game.board, p1_game.current_piece)) {
			//std::cout << "game lasted: " << turn << std::endl;
			game_over = true;
			return;
		}

		if (p2_game.collides(p2_game.board, p2_game.current_piece)) {
			//std::cout << "game lasted: " << turn << std::endl;
			game_over = true;
			return;
		}



		if (p1_move.null_move && p2_move.null_move) {
			p1_move.null_move = false;
			p2_move.null_move = false;
		}

		turn += 1;

		int p1_cleared_lines = 0;
		// player 1 move
		if (!p1_move.null_move) {
			spinType spin = p1_game.current_piece.spin;

			p1_game.place_piece(p1_move.piece);
			p1_cleared_lines = p1_game.board.clearLines();

			bool pc = true;
			for (int i = 0; i < Board::width; i++) {
				if (p1_game.board.get_column(i) != 0) {
					pc = false;
					break;
				}
			}

			if (spin == spinType::normal && p1_cleared_lines > 0) {
				//std::cout << "p1 did T-spin " << p1_cleared_lines << std::endl;
			}

			if (pc) {
				//std::cout << "p1 did perfect clear" << std::endl;
			}

			int dmg = p1_game.damage_sent(p1_cleared_lines, spin, pc);

			p1_atk += dmg;

			p2_meter += dmg;
		}
		int p2_cleared_lines = 0;
		// player 2 move
		if (!p2_move.null_move) {
			spinType spin = p2_game.current_piece.spin;

			p2_game.place_piece(p2_move.piece);
			p2_cleared_lines = p2_game.board.clearLines();

			bool pc = true;
			for (int i = 0; i < Board::width; i++) {
				if (p2_game.board.get_column(i) != 0) {
					pc = false;
					break;
				}
			}

			if (spin == spinType::normal && p2_cleared_lines > 0) {
				//std::cout << "p2 did T-spin " << p2_cleared_lines << std::endl;
			}

			if (pc) {
				//std::cout << "p2 did perfect clear" << std::endl;
			}

			int dmg = p2_game.damage_sent(p2_cleared_lines, spin, pc);

			p2_atk += dmg;

			p1_meter += dmg;
		}

		int min_meter = std::min(p1_meter, p2_meter);

		p1_meter -= min_meter;
		p2_meter -= min_meter;

		// if possible for both players to have damage, update here
		if (!p1_move.null_move && p1_cleared_lines == 0)
		{
			if (p1_meter > 0)
			{
				p1_game.add_garbage(p1_meter, p1_rng.GetRand(10));
				p1_meter = 0;
			}
		}

		if (!p2_move.null_move && p2_cleared_lines == 0)
		{
			if (p2_meter > 0)
			{
				p2_game.add_garbage(p2_meter, p2_rng.GetRand(10));
				p2_meter = 0;
			}
		}

		p1_game.queue.back() = p1_rng.GetPiece();
		p2_game.queue.back() = p2_rng.GetPiece();
	}


    inline std::vector<Move> get_moves(int id) const
	{
		std::vector<Move> moves;

		const Game& player = id == 0 ? p1_game : p2_game;

		std::vector<Piece> movegen_pieces = player.movegen(player.current_piece.type);

		PieceType hold = player.hold.has_value() ? player.hold.value().type : player.queue.front();

		std::vector<Piece> hold_pieces;
		if (hold != PieceType::Empty)
			hold_pieces = player.movegen(hold);

		moves.reserve((movegen_pieces.size() + hold_pieces.size()) * 2);

		for (auto& piece : movegen_pieces) {
			moves.emplace_back(piece, false);
			moves.emplace_back(piece, true);
		}

		for (auto& piece : hold_pieces) {
			moves.emplace_back(piece, false);
			moves.emplace_back(piece, true);
		}

		return moves;
	}



    inline Outcomes get_winner() const
	{

		Outcomes out = Outcomes::NONE;

		if (p1_game.collides(p1_game.board, p1_game.current_piece)) {
			out = Outcomes::P2_WIN;
		}

		if (p2_game.collides(p2_game.board, p2_game.current_piece)) {
			if (out == Outcomes::P2_WIN) {
				out = Outcomes::DRAW;
			}
			else {
				out = Outcomes::P1_WIN;
			}
		}

		return out;
	}

};