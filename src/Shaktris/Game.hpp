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


class Game {

public:
    static constexpr int queue_size = 5;
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

    bool place_piece(Piece& piece);

    bool collides(const Board& board, const Piece& piece) const;

    void rotate(Piece& piece, TurnDirection dir) const;

    void shift(Piece& piece, int dir) const;

    void sonic_drop(const Board& board, Piece& piece) const;

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

    std::array<PieceType, queue_size> queue;
};