#pragma once
#include <algorithm>

#include "Bot.hpp"
#include "VersusGame.hpp"
#include "Window.hpp"
#include "inputs.hpp"

using Rect = SDL_Rect;

using Point = SDL_Point;
using FPoint = SDL_FPoint;
using Color = SDL_Color;

constexpr float board_aspect_ratio = 14. / 20.;
class Stadium {
   private:
    VersusGame game;

    bool running = false;

    // pieces per second that the bots will play at
    float pps = 2.0;
    // the frame count for the bots to determine when they should move
    int frameCount = 0;

    Bot player_1;
    Bot player_2;

   public:
    inline bool update(Shakkar::inputs& inputs) {
        if (!running) {
            if (inputs.getKey(SDLK_RETURN).pressed) {
                if (player_1.running && player_2.running) {
                    game = VersusGame();
                    running = true;
                }
            }
        }

        if (running) {
            if (game.game_over) {
                running = false;
                return true;
            }

            // if need to move then ask the bots for moves
            if (frameCount >= UPDATES_PER_SECOND / pps) {
                // TODO
                frameCount = 0;
            }

            frameCount++;
        }

        return true;
    }

   private:
    inline Color get_color(PieceType type) {
        switch (type) {
            case PieceType::I:
                return {0, 255, 255, 255};
            case PieceType::J:
                return {0, 0, 255, 255};
            case PieceType::L:
                return {255, 165, 0, 255};
            case PieceType::O:
                return {255, 255, 0, 255};
            case PieceType::S:
                return {0, 255, 0, 255};
            case PieceType::T:
                return {128, 0, 128, 255};
            case PieceType::Z:
                return {255, 0, 0, 255};
            default:
                return {0, 0, 0, 0};
        }
    }

    inline Rect getInnerRect(Rect parent, float aspect_ratio) {
        int height, width;
        if ((float)parent.w / parent.h > aspect_ratio) {
            height = std::min(parent.h, int(parent.w / aspect_ratio));
            width = int(height * aspect_ratio);
        } else {
            width = std::min(parent.w, int(parent.h * aspect_ratio));
            height = int(width / aspect_ratio);
        }

        Rect board = {parent.x + (parent.w - width) / 2, parent.y + (parent.h - height) / 2, width, height};

        return board;
    }

    inline void draw_board_background(Window& window, Rect board) {
        // first two columns are for rendering the hold
        // the next 10 columns are for the board
        // the last two columns are for the queue

        FPoint cell_size = {board.w / 14.0f, board.h / 20.0f};

        // draw filled board
        {
            window.setDrawColor(0, 0, 0, 255);
            Rect board = {board.x + 2 * cell_size.x, board.y, 10 * cell_size.x, 20 * cell_size.y};
            window.drawRectFilled(board);
        }

        window.setDrawColor(255, 255, 255, 255);
        for (int i = 2; i < 12; i++) {
            for (int j = 0; j < 20; j++) {
                Rect cell = {board.x + i * cell_size.x, board.y + j * cell_size.y, ceil(cell_size.x), ceil(cell_size.y)};
                window.drawRect(cell);
            }
        }

        // draw hold background
        {
            window.setDrawColor(0, 0, 0, 255);
            Rect hold = {board.x, board.y + cell_size.y, 2 * cell_size.x, 2 * cell_size.y};
            window.drawRectFilled(hold);

            window.setDrawColor(255, 255, 255, 255);
            window.drawRect(hold);
        }
    }

    inline void draw_board(Window& window, Rect board, Board& b) {
        FPoint cell_size = {board.w / 14.0, board.h / 20.0};
        Point board_point = {board.x + cell_size.x * 2, board.y + board.h};

        window.setDrawColor(100, 100, 100, 255);

        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 20; j++) {
                if (b.get(i, j) != 0) {
                    Rect cell = {int(ceil(board_point.x + i * cell_size.x)), int(ceil(board_point.y - (j + 1) * cell_size.y)), (int)ceil(cell_size.x), (int)ceil(cell_size.y)};
                    window.drawRectFilled(cell);
                }
            }
        }
    }

    void draw_piece(Window& window, Rect area, Piece& piece) {
        FPoint cell_size = {area.w / 14.0f, area.h / 20.0f};
        Rect board = {
            area.x + 2.0f * cell_size.x,
            area.y,
            10.0f * cell_size.x,
            20.0f * cell_size.y};
        Point board_point = {board.x, board.y + board.h};
        Color col = get_color(piece.type);
        window.setDrawColor(col.r, col.g, col.b, col.a);

        for (auto& mino : piece.minos) {
            Rect cell = {
                int(ceil(board_point.x + (piece.position.x + mino.x) * cell_size.x)),
                int(ceil(board_point.y - (piece.position.y + mino.y) * cell_size.y)),
                (int)ceil(cell_size.x), (int)ceil(cell_size.y)};

            window.drawRectFilled(cell);
        }
    }

   public:
    inline void render(Window& window) {
        int windowWidth{}, windowHeight{};
        window.getWindowSize(windowWidth, windowHeight);

        window.clear();

        window.push_color(255, 0, 0, 255);

        // draw player 1

        Rect p1_zone = {int(windowWidth * 0.01), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98)};
        window.drawRectFilled(p1_zone);
        Rect p1_area = getInnerRect(p1_zone, board_aspect_ratio);
        draw_board_background(window, p1_area);
        draw_piece(window, p1_area, game.p1_game.current_piece);
        draw_board(window, p1_area, game.p1_game.board);

        // draw player 2

        window.setDrawColor(60, 50, 200, 255);
        Rect p2 = {int(windowWidth * 0.51), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98)};
        window.drawRectFilled(p2);
        Rect p2_area = getInnerRect(p2, board_aspect_ratio);
        draw_board_background(window, p2_area);
        draw_piece(window, p2_area, game.p2_game.current_piece);
        draw_board(window, p2_area, game.p2_game.board);

        window.pop_color();
        window.display();
    }
};