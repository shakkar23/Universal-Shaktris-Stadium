#include "../Shaktris/Board.hpp"
#include <cstdint>

using u8 = uint8_t;


struct game_state_datum {

    // board
    std::array<u8, 10*20> b;

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
};
