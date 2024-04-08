#pragma once

#include <array>
#include <cstdint>

using u8 = uint8_t;    ///<   8-bit unsigned integer.
using u16 = uint16_t;  ///<  16-bit unsigned integer.
using u32 = uint32_t;  ///<  32-bit unsigned integer.
using u64 = uint64_t;  ///<  64-bit unsigned integer.

enum class spinType {
    null,
    mini,
    normal,
};
struct Coord {
    int8_t x;
    int8_t y;
};

enum RotationDirection : u8 {
    North,
    East,
    South,
    West,
    RotationDirections_N
};

enum ColorType : u8 {
    // Color for pieces
    S,
    Z,
    J,
    L,
    T,
    O,
    I,
    // special types
    Empty,
    Garbage
};

enum class PieceType : u8 {
    // actual pieces
    S = ColorType::S,
    Z = ColorType::Z,
    J = ColorType::J,
    L = ColorType::L,
    T = ColorType::T,
    O = ColorType::O,
    I = ColorType::I,
    Empty = ColorType::Empty,
};

enum TurnDirection : u8 {
    Left,
    Right,
};

enum class Movement : u8 {
    Left,
    Right,
    RotateClockwise,
    RotateCounterClockwise,
    SonicDrop,
};

// inputs supported by this implementation
enum class inputs {
    RotateLeft,
    RotateRight,
    Left,
    Right,
    SonicDrop,
    Hold,
};

// number of kicks srs has, including for initial
constexpr const int srs_kicks = 5;

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_JLSTZ = { {
    {{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}},
    {{{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}},
    {{{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}}},
    {{{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}},
} };

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_O = { {
    {{{0, 0}}},
    {{{0, -1}}},
    {{{-1, -1}}},
    {{{-1, 0}}},
} };

constexpr std::array<std::array<Coord, srs_kicks>, RotationDirections_N> piece_offsets_I = { {

    {{{0, 0}, {-1, 0}, {2, 0}, {-1, 0}, {2, 0}}},
    {{{-1, 0}, {0, 0}, {0, 0}, {0, 1}, {0, -2}}},
    {{{-1, 1}, {1, 1}, {-2, 1}, {1, 0}, {-2, 0}}},
    {{{0, 1}, {0, 1}, {0, 1}, {0, -1}, {0, 2}}},
} };

constexpr std::array<std::array<Coord, 4>, 8> piece_definitions = {
    {
        {{{-1, 0}, {0, 0}, {0, 1}, {1, 1}}},   // S
        {{{-1, 1}, {0, 1}, {0, 0}, {1, 0}}},   // Z
        {{{-1, 0}, {0, 0}, {1, 0}, {-1, 1}}},  // J
        {{{-1, 0}, {0, 0}, {1, 0}, {1, 1}}},   // L
        {{{-1, 0}, {0, 0}, {1, 0}, {0, 1}}},   // T
        {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},    // O
        {{{-1, 0}, {0, 0}, {1, 0}, {2, 0}}},   // I
        {{{0, 0}, {0, 0}, {0, 0}, {0, 0}}}     // NULL
    } 
};
