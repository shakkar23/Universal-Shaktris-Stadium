#pragma once
#include "Constants.hpp"
#include<stdint.h>

#include <array>
#include <algorithm>

using u8  = uint8_t;  ///<   8-bit unsigned integer.
using u16 = uint16_t; ///<  16-bit unsigned integer.
using u32 = uint32_t; ///<  32-bit unsigned integer.
using u64 = uint64_t; ///<  64-bit unsigned integer.

class RNG {

public:
    constexpr RNG() {
        makebag();
    }
    u32 rng = 0;
    std::array<PieceType, 7> bag;
    u8 bagiterator{};
    constexpr inline u32 GetRand(u32 upperBound) {
        rng = rng * 0x5d588b65 + 0x269ec3;
        u32 uVar1 = rng >> 0x10;
        if (upperBound != 0) {
            uVar1 = (uVar1 * upperBound) >> 0x10;
        }
        return uVar1;
    }

    constexpr inline void makebag() {
        bagiterator = 0;
        u8 buffer = 0;
        std::array<PieceType, 7> pieces = { PieceType::S, PieceType::Z, PieceType::J, PieceType::L, PieceType::T, PieceType::O, PieceType::I, };
        for (int_fast8_t i = 6; i >= 0; i--) {
            buffer = (u32)GetRand(i + 1);
            bag[i] = pieces[buffer];
            std::swap(pieces[buffer], pieces[i]);
        }
    }

    // returns the next piece 
    constexpr inline PieceType GetPiece() {
        PieceType next = bag[bagiterator];
        if (bagiterator == 6)
            makebag();
        else
            bagiterator++;
        return next;
    }
};


