#pragma once
#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

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

    int windowWidth{}, windowHeight{};

   public:
    inline bool update(Shakkar::inputs& inputs) {
        if (!running) {
            if (inputs.getKey(SDLK_RETURN).pressed) {
                if (player_1.running && player_2.running) {
                    game = VersusGame();

                    running = true;

                    player_1.TBP_start(game.p1_game.board, game.p1_game.queue);
                    player_2.TBP_start(game.p2_game.board, game.p2_game.queue);

                    // wait for the bots to start
                    std::this_thread::sleep_for(std::chrono::seconds(5));

                    player_1.TBP_suggest();
                    std::vector<Piece> suggestion_1 = player_1.TBP_suggestion();

                    player_2.TBP_suggest();
                    std::vector<Piece> suggestion_2 = player_2.TBP_suggestion();

                    if (suggestion_1.size() > 0 && suggestion_2.size() > 0) {
                        Piece move = suggestion_1[0];

                        if (move.type != game.p1_game.current_piece.type) {
                            if (game.p1_game.hold) {
                                game.p1_game.do_hold();
                            } else {
                                game.p1_game.do_hold();
                                game.p1_game.queue.back() = game.p1_rng.GetPiece();
                            }
                        }
                        game.p1_game.current_piece = move;
                        game.p1_game.place_piece();
                    }
                }
            }
            std::string file = inputs.getDroppedFile();
            if (!file.empty()) {
                if (inputs.getMouse().x < windowWidth / 2) {
                    player_1.start(file.c_str());
                } else {
                    player_2.start(file.c_str());
                }
            }
        }

        if (running) {
            if (game.game_over) {
                running = false;
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

    inline void draw_board_background(Window& window, Rect area) {
        // first two columns are for rendering the hold
        // the next 10 columns are for the board
        // the last two columns are for the queue

        FPoint cell_size = {area.w / 14.0f, area.h / 20.0f};

        // draw filled board
        {
            window.setDrawColor(0, 0, 0, 255);
            Rect board = {
                int(area.x + 2 * cell_size.x),
                int(area.y),
                int(10 * cell_size.x),
                int(20 * cell_size.y)};
            window.drawRectFilled(board);
        }

        window.setDrawColor(255, 255, 255, 255);
        for (int i = 2; i < 12; i++) {
            for (int j = 0; j < 20; j++) {
                Rect cell = {
                    int(area.x + i * cell_size.x),
                    int(area.y + j * cell_size.y),
                    int(ceil(cell_size.x)),
                    int(ceil(cell_size.y))};
                window.drawRect(cell);
            }
        }
    }
    inline void draw_stasis_piece(Window& window, Rect area, std::optional<Piece> piece) {
        // assume that the area is the hold area and we render the piece with (0, 0) as the center, the grid being a 5x5 grid
        FPoint cell_size = {area.w / 5.0f, area.h / 5.0f};

        window.setDrawColor(0, 0, 0, 255);
        window.drawRectFilled(area);

        window.setDrawColor(255, 255, 255, 255);
        window.drawRect(area);

        if (!piece.has_value()) {
            return;
        }
        Color col = get_color(piece.value().type);
        window.setDrawColor(col.r, col.g, col.b, col.a);
        for (auto& mino : piece.value().minos) {
            Rect cell = {
                int(ceil(area.x + (2 + mino.x) * cell_size.x)),
                int(ceil(area.y + (2 - mino.y) * cell_size.y)),
                (int)ceil(cell_size.x), (int)ceil(cell_size.y)};

            window.drawRectFilled(cell);
        }
    }

    inline void draw_board(Window& window, Rect board, Board& b) {
        FPoint cell_size = {board.w / 14.0f, board.h / 20.0f};
        Point board_point = {int(board.x + cell_size.x * 2), int(board.y + board.h)};

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

    void draw_piece(Window& window, Rect area, Piece piece) {
        FPoint cell_size = {area.w / 14.0f, area.h / 20.0f};
        Rect board = {
            int(area.x + 2.0f * cell_size.x),
            int(area.y),
            int(10.0f * cell_size.x),
            int(20.0f * cell_size.y)};
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
        draw_hold_and_queue(window, game.p1_game, p1_area);

        // draw player 2

        window.setDrawColor(60, 50, 200, 255);
        Rect p2 = {int(windowWidth * 0.51), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98)};
        window.drawRectFilled(p2);
        Rect p2_area = getInnerRect(p2, board_aspect_ratio);
        draw_board_background(window, p2_area);
        draw_piece(window, p2_area, game.p2_game.current_piece);
        draw_board(window, p2_area, game.p2_game.board);
        draw_hold_and_queue(window, game.p2_game, p2_area);

        window.pop_color();
        window.display();
    }
    void draw_hold_and_queue(Window& window, Game& player, Rect& p1_area) {
        {
            FPoint cell_size = {p1_area.w / 14.0f, p1_area.h / 20.0f};
            Rect hold = {
                int(p1_area.x),
                int(p1_area.y + cell_size.y),
                int(2 * cell_size.x),
                int(2 * cell_size.y)};

            draw_stasis_piece(window, hold, player.hold);

            Rect queue = {
                int(p1_area.x + 12 * cell_size.x),
                int(ceil(p1_area.y + 2 * cell_size.y)),
                int(2 * cell_size.x),
                int(ceil(2 * 5 * cell_size.y))};

            for (int i = 0; i < 5; i++) {
                std::optional<Piece> queue_piece = Piece(player.queue[i]);
                draw_stasis_piece(window, {queue.x, int(ceil(queue.y + i * 2 * cell_size.y)), queue.w, int(ceil(2 * cell_size.y))}, queue_piece);
            }
        }
    }
};