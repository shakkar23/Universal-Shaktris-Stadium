#pragma once
#include <array>
#include <cmath>
#include <optional>
#include <ranges>
#include <vector>

#include "Board.hpp"
#include "Move.hpp"
#include "Piece.hpp"
#include "Constants.hpp"
#include "rng.hpp"
#include "tetrio.hpp"

constexpr int QUEUE_SIZE = 5;

class Game {
public:
    Game() : current_piece(PieceType::Empty) {
        for (auto& p : queue) {
            p = PieceType::Empty;
        }
    }
    Game& operator=(const Game& other) {
        if (this != &other) {
            board = other.board;
            current_piece = other.current_piece;
            hold = other.hold;
            garbage_meter = other.garbage_meter;
            stats = other.stats;
            queue = other.queue;
        }
        return *this;
    }
    ~Game() {}

    void place_piece();

    void place_piece(Piece& piece);

    void do_hold();

    bool collides(const Board& board, const Piece& piece) const;

    void rotate(Piece& piece, TurnDirection dir) const;

    void shift(Piece& piece, int dir) const;

    void sonic_drop(const Board board, Piece& piece) const;

    void add_garbage(int lines, int location);

    int damage_sent(int linesCleared, spinType spinType, bool pc);

    void process_movement(Piece& piece, Movement movement) const;

    std::vector<Piece> movegen(PieceType piece_type) const;

    std::vector<Piece> get_possible_piece_placements() const;

    Board board;
    Piece current_piece;
    std::optional<Piece> hold;
    int garbage_meter = 0;

    TetrioStats stats;

    std::array<PieceType, QUEUE_SIZE> queue;
};