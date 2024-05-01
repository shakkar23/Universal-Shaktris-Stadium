#pragma once

#include <optional>

#include "Piece.hpp"
#include "Constants.hpp"

class Move {
public:
    Move() : piece(Piece(PieceType::Empty)) {
        this->null_move = true;
    }

    Move(const Piece& piece, bool null_move) : piece(piece) {
        this->null_move = null_move;
    }

    Move(const std::optional<Piece>& piece) : piece(Piece(PieceType::Empty)) {
        if (!piece) {
            this->null_move = true;
        }
        else {
            this->piece = *piece;
            this->null_move = false;
        }
    }

    Piece piece;
    bool null_move;
    
};