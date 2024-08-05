#include "Game.hpp"

#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <random>
#include <ranges>
#include <tuple>

void Game::place_piece() {
    board.set(current_piece);

    current_piece = queue.front();

    std::shift_left(queue.begin(), queue.end(), 1);

    queue.back() = PieceType::Empty;
}


bool Game::place_piece(Piece& piece) {
    bool first_hold = false;
    if (piece.type != current_piece.type) {
        if (!hold.has_value())
        {  // shift queue
            std::shift_left(queue.begin(), queue.end(), 1);

            queue.back() = PieceType::Empty;

            first_hold = true;
        }
        hold = current_piece;
    }

    current_piece = piece;
    place_piece();

    return first_hold;
}

bool Game::collides(const Board& board, const Piece& piece) const {
    for (auto& mino : piece.minos) {
        int x_pos = mino.x + piece.position.x;
        if (x_pos < 0 || x_pos >= Board::width)
            return true;

        int y_pos = mino.y + piece.position.y;
        if (y_pos < 0 || y_pos >= Board::height)
            return true;
        if (board.get(x_pos, y_pos))
            return true;
    }

    return false;
}

void Game::rotate(Piece& piece, TurnDirection dir) const {
    const RotationDirection prev_rot = piece.rotation;

    piece.rotate(dir);

    const std::array<Coord, srs_kicks>* offsets = &piece_offsets_JLSTZ[piece.rotation];
    const std::array<Coord, srs_kicks>* prev_offsets = &piece_offsets_JLSTZ[prev_rot];

    if (piece.type == PieceType::I) {
        prev_offsets = &piece_offsets_I[prev_rot];
        offsets = &piece_offsets_I[piece.rotation];
    }
    else if (piece.type == PieceType::O) {
        prev_offsets = &piece_offsets_O[prev_rot];
        offsets = &piece_offsets_O[piece.rotation];
    }

    auto x = piece.position.x;
    auto y = piece.position.y;

    for (int i = 0; i < srs_kicks; i++) {
        piece.position.x = x + (*prev_offsets)[i].x - (*offsets)[i].x;
        piece.position.y = y + (*prev_offsets)[i].y - (*offsets)[i].y;
        if (!collides(board, piece)) {
            if (piece.type == PieceType::T) {
                constexpr std::array<std::array<Coord, 4>, 4> corners = { {
                        // a       b       c        d
                        {{{-1, 1}, {1, 1}, {1, -1}, {-1, -1}}},  // North
                        {{{1, 1}, {1, -1}, {-1, -1}, {-1, 1}}},  // East
                        {{{1, -1}, {-1, -1}, {-1, 1}, {1, 1}}},  // South
                        {{{-1, -1}, {-1, 1}, {1, 1}, {1, -1}}},  // West
                    } };

                bool filled[4] = { true, true, true, true };

                for (int u = 0; u < 4; u++) {
                    Coord c = corners[piece.rotation][u];
                    c.x += piece.position.x;
                    c.y += piece.position.y;
                    if (c.x >= 0 && c.x < Board::width)
                        filled[u] = board.get(c.x, c.y);
                }

                if (filled[0] && filled[1] && (filled[2] || filled[3]))
                    piece.spin = spinType::normal;
                else if ((filled[0] || filled[1]) && filled[2] && filled[3]) {
                    if (i >= (srs_kicks - 1)) {
                        piece.spin = spinType::normal;
                    }
                    else
                        piece.spin = spinType::mini;
                }
                else
                    piece.spin = spinType::null;
            }
            return;
        }
    }

    piece.position.x = x;
    piece.position.y = y;

    piece.rotate(dir == TurnDirection::Left ? TurnDirection::Right : TurnDirection::Left);
}

void Game::shift(Piece& piece, int dir) const {
    piece.position.x += dir;

    if (collides(board, piece))
        piece.position.x -= dir;
    else
        piece.spin = spinType::null;
}

void Game::sonic_drop(const Board& board, Piece& piece) const {
    int distance = 32;
    for (auto& mino : piece.minos) {

        int mino_height = mino.y + piece.position.y;

        uint32_t column = board.board[mino.x + piece.position.x];

        if (column && mino_height != 0) {
            int air = 32 - mino_height;
            mino_height -= 32 - std::countl_zero((column << air) >> air);
        }

        distance = std::min(distance, mino_height);
    }

    piece.position.y -= distance;
}

void Game::add_garbage(int lines, int location) {
    for (int i = 0; i < Board::width; ++i) {
        auto& column = board.board[i];

        column <<= lines;

        if (location != i) {
            column |= (1 << lines) - 1;
        }
    }
}

// ported from
// https://github.com/emmachase/tetrio-combo
int Game::damage_sent(int linesCleared, spinType spinType, bool pc) {
    return tetrio_damage({ linesCleared, spinType, pc }, stats);
}

void Game::process_movement(Piece& piece, Movement movement) const {
    switch (movement) {
    case Movement::Left:
        shift(piece, -1);
        break;
    case Movement::Right:
        shift(piece, 1);
        break;
    case Movement::RotateClockwise:
        rotate(piece, TurnDirection::Right);
        break;
    case Movement::RotateCounterClockwise:
        rotate(piece, TurnDirection::Left);
        break;
    case Movement::SonicDrop:
        sonic_drop(board, piece);
        break;
        // default:
        // std::unreachable();
    }
}

std::vector<Piece> Game::movegen(PieceType piece_type) const {
    Piece initial_piece = Piece(piece_type);

    std::vector<Piece> open_nodes;
    open_nodes.reserve(150);
    std::vector<Piece> next_nodes;
    next_nodes.reserve(150);
    std::vector<bool> visited = std::vector<bool>(6444);

    std::vector<Piece> valid_moves;
    valid_moves.reserve(100);

    // root node
    open_nodes.emplace_back(initial_piece);

    while (open_nodes.size() > 0) {
        // expand edges
        for (auto& piece : open_nodes) {
            auto h = piece.compact_hash();
            if (visited[h])
                continue;
            // mark node as visited
            visited[h] = true;

            // try all movements
            Piece new_piece = piece;
            process_movement(new_piece, Movement::RotateCounterClockwise);
            next_nodes.emplace_back(new_piece);

            new_piece = piece;
            process_movement(new_piece, Movement::RotateClockwise);
            next_nodes.emplace_back(new_piece);

            new_piece = piece;
            process_movement(new_piece, Movement::Left);
            next_nodes.emplace_back(new_piece);

            new_piece = piece;
            process_movement(new_piece, Movement::Right);
            next_nodes.emplace_back(new_piece);

            new_piece = piece;
            process_movement(new_piece, Movement::SonicDrop);
            next_nodes.emplace_back(new_piece);

            // check if the piece is grounded and therefore valid

            piece.position.y--;

            if (collides(board, piece)) {
                piece.position.y++;
                valid_moves.emplace_back(piece);
            }
        }
        open_nodes = next_nodes;
        next_nodes.clear();
    }

    return valid_moves;
}

// warning! if there is a piece in the hold and the current piece is empty, we dont use the hold
std::vector<Piece> Game::get_possible_piece_placements() const {

    std::vector<Piece> valid_pieces = movegen(current_piece.type);

    PieceType holdType = hold.has_value() ? hold->type : queue.front();

    std::vector<Piece> hold_pieces = movegen(holdType);
    valid_pieces.reserve(valid_pieces.size() + hold_pieces.size());


    for (auto& piece : hold_pieces) {
        valid_pieces.emplace_back(piece);
    }

    return valid_pieces;
}