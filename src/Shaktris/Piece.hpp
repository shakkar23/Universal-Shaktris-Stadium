#pragma once
#include <vector>
#include <cstddef>

#include "Constants.hpp"

class Piece {
public:
    Piece(PieceType type) {
        this->type = type;
        rotation = RotationDirection::North;
        position = { 10 / 2 - 1, 20 - 1 };
        minos = piece_definitions[static_cast<size_t>(type)];
        spin = spinType::null;
    }
    Piece(PieceType type, Coord position, RotationDirection rotation, spinType spin) {
        this->type = type;
        this->position = position;
        this->rotation = rotation;
        this->spin = spin;
        minos = piece_definitions[static_cast<size_t>(type)];

        for (int i = 0; i < static_cast<int>(rotation); i++) {
            rotate(TurnDirection::Right);
        }
    }

    Piece() = delete;
    Piece(const Piece& other) = default;
    Piece(Piece&& other) noexcept = default;
    ~Piece() = default;
    Piece& operator=(const Piece& other) = default;

    inline void rotate(TurnDirection direction) {
        if (direction == TurnDirection::Left) {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + 3) % 4);
            for (auto& mino : minos) {
                Coord temp_mino = mino;
                temp_mino.y *= -1;
                mino = { temp_mino.y, temp_mino.x };
            }
        }
        else {
            rotation = static_cast<RotationDirection>((static_cast<int>(rotation) + 1) % 4);
            for (auto& mino : minos) {
                Coord temp_mino = mino;
                temp_mino.x *= -1;
                mino = { temp_mino.y, temp_mino.x };
            }
        }
    }

    inline uint32_t compact_hash() const {
        return rotation + position.x * 4 + position.y * 10 * 4 + (int)type * 10 * 20 * 4;
    }


    std::array<Coord, 4> minos;
    Coord position;
    RotationDirection rotation;
    PieceType type;
    spinType spin;
};