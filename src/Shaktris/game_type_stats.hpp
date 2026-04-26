#pragma once

#include "Constants.hpp"
#include <algorithm>
#include <cmath>

enum damage_type {
    classic_guideline,
    modern_guideline
};
namespace GarbageValues {
    const int SINGLE = 0;
    const int DOUBLE = 1;
    const int TRIPLE = 2;
    const int QUAD = 4;
    const int TSPIN_MINI = 0;
    const int TSPIN = 0;
    const int TSPIN_MINI_SINGLE = 0;
    const int TSPIN_SINGLE = 2;
    const int TSPIN_MINI_DOUBLE = 1;
    const int TSPIN_DOUBLE = 4;
    const int TSPIN_TRIPLE = 6;
    const int TSPIN_QUAD = 10;
    const int BACKTOBACK_BONUS = 1;
    const float BACKTOBACK_BONUS_LOG = .8f;
    const int COMBO_MINIFIER = 1;
    const float COMBO_MINIFIER_LOG = 1.25;
    const float COMBO_BONUS = .25;
    const int ALL_CLEAR = 10;
    const std::array<std::array<int, 13>, 2> combotable = { {{0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5},
                                                            {0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4}} };
}  // namespace GarbageValues

constexpr struct tetrio_settings {
    int garbagemultiplier = 1;
    bool b2bchaining = true;
} game_settings;

struct Params {
    // per piece basis
    const int linesCleared;
    const spinType spin;
    const bool pc;
};

struct TetrioStats {
    // game state
    int combo = 0;
    int b2b = 0;
    int currentcombopower = 0;
    int currentbtbchainpower = 0;

    inline int damage(Params params) {
        bool maintainsB2B = false;
        if (params.linesCleared) {
            this->combo++;
            if (4 == params.linesCleared || params.spin != spinType::null) {
                maintainsB2B = true;
            }

            if (maintainsB2B) {
                this->b2b++;
            }
            else {
                this->b2b = 0;
            }
        }
        else {
            this->combo = 0;
            this->currentcombopower = 0;
        }

        int garbage = 0;

        switch (params.linesCleared) {
        case 0:
            if (spinType::mini == params.spin) {
                garbage = GarbageValues::TSPIN_MINI;
            }
            else if (spinType::normal == params.spin) {
                garbage = GarbageValues::TSPIN;
            }
            break;
        case 1:
            if (spinType::mini == params.spin) {
                garbage = GarbageValues::TSPIN_MINI_SINGLE;
            }
            else if (spinType::normal == params.spin) {
                garbage = GarbageValues::TSPIN_SINGLE;
            }
            else {
                garbage = GarbageValues::SINGLE;
            }
            break;

        case 2:
            if (spinType::mini == params.spin) {
                garbage = GarbageValues::TSPIN_MINI_DOUBLE;
            }
            else if (spinType::normal == params.spin) {
                garbage = GarbageValues::TSPIN_DOUBLE;
            }
            else {
                garbage = GarbageValues::DOUBLE;
            }
            break;

        case 3:
            if (params.spin != spinType::null) {
                garbage = GarbageValues::TSPIN_TRIPLE;
            }
            else {
                garbage = GarbageValues::TRIPLE;
            }
            break;

        case 4:
            if (params.spin != spinType::null) {
                garbage = GarbageValues::TSPIN_QUAD;
            }
            else {
                garbage = GarbageValues::QUAD;
            }
            break;
        }

        if (params.linesCleared) {
            if (this->b2b > 1) {
                if (game_settings.b2bchaining) {
                    const int b2bGarbage = GarbageValues::BACKTOBACK_BONUS *
                        (1 + std::log1p((this->b2b - 1) * GarbageValues::BACKTOBACK_BONUS_LOG) + (this->b2b - 1 <= 1 ? 0 : (1 + std::log1p((this->b2b - 1) * GarbageValues::BACKTOBACK_BONUS_LOG) - (int)std::log1p((this->b2b - 1) * GarbageValues::BACKTOBACK_BONUS_LOG)) / 3));

                    garbage += b2bGarbage;

                    if (b2bGarbage > this->currentbtbchainpower) {
                        this->currentbtbchainpower = b2bGarbage;
                    }
                }
                else {
                    garbage += GarbageValues::BACKTOBACK_BONUS;
                }
            }
            else {
                this->currentbtbchainpower = 0;
            }
        }

        if (this->combo > 1) {
            garbage *= 1 + GarbageValues::COMBO_BONUS * (this->combo - 1);  // Fucking broken ass multiplier :)
        }

        if (this->combo > 2) {
            garbage = std::max((int)std::log1p(GarbageValues::COMBO_MINIFIER * (this->combo - 1) * GarbageValues::COMBO_MINIFIER_LOG), garbage);
        }

        const int finalGarbage = garbage * game_settings.garbagemultiplier;
        if (this->combo > 2) {
            this->currentcombopower = std::max(this->currentcombopower, finalGarbage);
        }

        const auto combinedGarbage = finalGarbage + (params.pc ? GarbageValues::ALL_CLEAR : 0);

        return combinedGarbage;
    }
};

struct PptStats {
    int combo{};
    int b2b{};
    constexpr static std::array combo_dmg = {0,0,1,1,2,2,3,3,4,4,4,5};
    constexpr static int perfect_clear_dmg = 10;

    inline int damage(Params params) {
        int sent{};
        if(params.linesCleared > 0) {
            if(params.spin != spinType::null) {
                switch(params.spin) {
                    case spinType::mini: {
                        sent = params.linesCleared - 1;
                    } break;
                    case spinType::normal: {
                        sent = params.linesCleared * 2;
                    } break;
                }
                sent += b2b;
                b2b = 1;
            } else if(params.linesCleared == 4) {
                sent = 4;
                sent += b2b;
                b2b = 1;
            } else {
                sent = params.linesCleared - 1;
                b2b = 0;
            }

            sent += combo_dmg[std::clamp(params.linesCleared, 0, (int)combo_dmg.size()-1)];
            combo++;

            int perfect_clear_damage = params.pc ? perfect_clear_dmg : 0;

        } else {
            combo = 0;
        }
        return sent;
    }
};

