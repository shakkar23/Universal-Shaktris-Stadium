#pragma once
#include <algorithm>
#include <optional>
#include <random>
#include <vector>

#include "Game.hpp"
#include "Move.hpp"
#include "Piece.hpp"
#include "rng.hpp"

enum class game_type {
    PPT,
    Tetrio
};

class VersusGame {
   public:
    VersusGame(game_type type = game_type::Tetrio, int garbage_delay = 0) : garbage_delay(garbage_delay) {
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

    int p1_damage_sent = 0;
    int p2_damage_sent = 0;

    double p1_atk = 0;
    double p2_atk = 0;

    // garbage each player is due to receive that they can cancel or tank now
    int p1_meter = 0;
    int p2_meter = 0;

    // turns an attack waits before it joins the receiver's meter. 0 is instant, as in tetr.io's versus;
    // 1 holds it back a turn, as in botris. garbage in transit can still be cancelled
    int garbage_delay = 0;

    struct PendingGarbage {
        int lines;
        int release_turn;
    };
    // attacks heading for each player that haven't joined their meter yet
    std::vector<PendingGarbage> p1_pending;
    std::vector<PendingGarbage> p2_pending;

    bool p1_spun = false;
    bool p2_spun = false;

    bool p1_accepts_garbage = false;
    bool p2_accepts_garbage = false;

    int turn = 0;
    enum class State : u8 {
        PLAYING,
        P1_WIN,
        P2_WIN,
        DRAW,
    } state = State::PLAYING;
    bool game_over = false;

    // get attack per piece
    double inline get_app(int id) const {
        return id == 0 ? p1_atk / turn : p2_atk / turn;
    }

    double inline get_b2b(int id) const {
        return std::visit(
            [](auto &stat) {
                return stat.b2b;
            },
            id == 0 ? p1_game.stats : p2_game.stats
        );
    }

    const inline Game& get_game(int id) const {
        return id == 0 ? p1_game : p2_game;
    }

    void set_move(int id, Move move) {
        if (id == 0) {
            p1_move = move;
        } else {
            p2_move = move;
        }
    }

    void play_moves() {
        // game over conditions

        if (game_over) {
            return;
        }

        if (p1_move.null_move && p2_move.null_move) {
            p1_move.null_move = false;
            p2_move.null_move = false;
        }

        turn += 1;

        int p1_cleared_lines = 0;
        int p1_attack = 0;
        bool p1_first_hold = false;
        // player 1 move
        if (!p1_move.null_move) {
            // only score the spin the placement can really have
            p1_move.piece.spin = p1_game.validate_spin(p1_move.piece);
            spinType spin = p1_move.piece.spin;

            p1_first_hold = p1_game.place_piece(p1_move.piece);
            p1_cleared_lines = p1_game.board.clearLines();

            bool pc = true;
            for (int i = 0; i < Board::width; i++) {
                if (p1_game.board.get_column(i) != 0) {
                    pc = false;
                    break;
                }
            }

            if (spin == spinType::normal || spin == spinType::mini && p1_cleared_lines > 0) {
                p1_spun = true;
            }
            else
                p1_spun = false;

            if (pc) {
                // std::cout << "p1 did perfect clear" << std::endl;
            }

            int dmg = p1_game.damage_sent(p1_cleared_lines, spin, pc);

            p1_damage_sent = dmg;

            p1_atk += dmg;

            p1_attack = dmg;
        } else 
            p1_spun = false;

        int p2_cleared_lines = 0;
        int p2_attack = 0;
        bool p2_first_hold = false;
        // player 2 move
        if (!p2_move.null_move) {
            // only score the spin the placement can really have
            p2_move.piece.spin = p2_game.validate_spin(p2_move.piece);
            spinType spin = p2_move.piece.spin;

            p2_first_hold = p2_game.place_piece(p2_move.piece);
            p2_cleared_lines = p2_game.board.clearLines();

            bool pc = true;
            for (int i = 0; i < Board::width; i++) {
                if (p2_game.board.get_column(i) != 0) {
                    pc = false;
                    break;
                }
            }

            if (spin == spinType::normal || spin == spinType::mini && p2_cleared_lines > 0) {
                p2_spun = true;
            } else 
                p2_spun = false;

            if (pc) {
                // std::cout << "p2 did perfect clear" << std::endl;
            }

            int dmg = p2_game.damage_sent(p2_cleared_lines, spin, pc);

            p2_damage_sent = dmg;

            p2_atk += dmg;

            p2_attack = dmg;
        } else 
			p2_spun = false;

        // garbage that has waited out the delay joins the meter, where it can be cancelled or tanked
        auto release = [this](std::vector<PendingGarbage>& pending, int& meter) {
            std::erase_if(pending, [&](const PendingGarbage& garbage) {
                if (garbage.release_turn > turn)
                    return false;
                meter += garbage.lines;
                return true;
            });
        };
        release(p1_pending, p1_meter);
        release(p2_pending, p2_meter);

        auto cancel = [](int& a, int& b) {
            int cancelled = std::min(a, b);
            a -= cancelled;
            b -= cancelled;
        };

        // zero pass-through: an attack cancels everything heading for its sender, soonest first,
        // the meter and then what's still in transit
        auto cancel_incoming = [&](int& attack, int& meter, std::vector<PendingGarbage>& pending) {
            cancel(attack, meter);
            for (auto& garbage : pending)
                cancel(attack, garbage.lines);
            std::erase_if(pending, [](const PendingGarbage& garbage) { return garbage.lines == 0; });
        };
        cancel_incoming(p1_attack, p1_meter, p1_pending);
        cancel_incoming(p2_attack, p2_meter, p2_pending);

        // and the rest of the two attacks meet head on
        cancel(p1_attack, p2_attack);

        if (garbage_delay == 0) {
            p2_meter += p1_attack;
            p1_meter += p2_attack;
        } else {
            if (p1_attack > 0)
                p2_pending.push_back({ p1_attack, turn + garbage_delay });
            if (p2_attack > 0)
                p1_pending.push_back({ p2_attack, turn + garbage_delay });
        }

        // a placement that clears nothing tanks the whole meter
        p1_accepts_garbage = false;
        if (!p1_move.null_move && p1_cleared_lines == 0 && p1_meter > 0) {
            p1_game.add_garbage(p1_meter, p1_rng.GetRand(10));
            p1_meter = 0;
            p1_accepts_garbage = true;
        }

        p2_accepts_garbage = false;
        if (!p2_move.null_move && p2_cleared_lines == 0 && p2_meter > 0) {
            p2_game.add_garbage(p2_meter, p2_rng.GetRand(10));
            p2_meter = 0;
            p2_accepts_garbage = true;
        }

        if (p1_first_hold)
            *(p1_game.queue.end() - 2) = p1_rng.GetPiece();
        p1_game.queue.back() = p1_rng.GetPiece();

        if (p2_first_hold)
            *(p2_game.queue.end() - 2) = p2_rng.GetPiece();
        p2_game.queue.back() = p2_rng.GetPiece();

        bool p1_died = false;
        if (p1_game.collides(p1_game.board, p1_game.current_piece)) {
            // std::cout << "game lasted: " << turn << std::endl;
            game_over = true;
            p1_died = true;
        }

        bool p2_died = false;
        if (p2_game.collides(p2_game.board, p2_game.current_piece)) {
            // std::cout << "game lasted: " << turn << std::endl;
            game_over = true;
            p2_died = true;
        }

        if (p1_died && p2_died) {
            state = State::DRAW;
        } else if (p1_died) {
            state = State::P2_WIN;
        } else if (p2_died) {
            state = State::P1_WIN;
        }

        return;
    }

    inline std::vector<Move> get_moves(int id) const {
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
};