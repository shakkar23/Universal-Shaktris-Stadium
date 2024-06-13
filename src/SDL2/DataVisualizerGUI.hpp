#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "Bot.hpp"
#include "VersusGame.hpp"
#include "Window.hpp"
#include "inputs.hpp"

using Rect = SDL_Rect;

using Point = SDL_Point;
using FPoint = SDL_FPoint;
using Color = SDL_Color;

constexpr float board_aspect_ratio = 14. / 20.;

class DataVisualizerGUI {
   public:
    DataVisualizerGUI() {
        game.p1_game.current_piece = Piece(PieceType::Empty);
        game.p2_game.current_piece = Piece(PieceType::Empty);

        for (auto& p : game.p1_game.queue) {
            p = PieceType::Empty;
        }
        for (auto& p : game.p2_game.queue) {
            p = PieceType::Empty;
        }
    }

   private:
    VersusGame game;
    std::fstream file;
    size_t file_size;
    int windowWidth{}, windowHeight{};
    bool file_loaded = false;

    struct data {
        data(const Game& game) {
            b = game.board;
            p_type = (u8)game.current_piece.type;
            p_rot = game.current_piece.rotation;
            p_x = (u8)game.current_piece.position.x;
            p_y = (u8)game.current_piece.position.y;
            meter = (u8)game.garbage_meter;
            queue[0] = (u8)game.queue[0];
            queue[1] = (u8)game.queue[1];
            queue[2] = (u8)game.queue[2];
            queue[3] = (u8)game.queue[3];
            queue[4] = (u8)game.queue[4];
            hold = game.hold.has_value() ? (u8)game.hold.value().type : 7;
        }
        data() = default;
        // board
        Board b;

        // piece
        u8 p_type;
        u8 p_rot;
        u8 p_x;
        u8 p_y;

        // extra data
        u8 meter;
        u8 queue[5];
        u8 hold;
    };

    void parse_data(bool prev) {
        if (prev) {
            // find the current position of the file cursor and move it back (1+52*2)*2 bytes and clamp the number from [0, file_size]
            auto pos = file.tellg();
            file.seekg((1 + 52 * 2) * -2, std::ios::cur);
            file.seekg(std::max(0, (int)file.tellg()), std::ios::beg);
        }
        // read the data from the file, reading one byte and then the data struct twice for each player

        // read the state
        VersusGame::State state;
        file.read((char*)&state, sizeof(VersusGame::State));

        // read the data for player 1
        data p1;
        file.read((char*)&p1, sizeof(data));

        // read the data for player 2
        data p2;
        file.read((char*)&p2, sizeof(data));

        // set the game state
        game.state = state;

        // set the game data
        game.p1_game.board = p1.b;
        game.p1_game.current_piece = Piece((PieceType)p1.p_type, Coord(p1.p_x, p1.p_y), (RotationDirection)p1.p_rot, spinType::null);
        game.p1_game.garbage_meter = p1.meter;
        game.p1_game.queue = {(PieceType)p1.queue[0], (PieceType)p1.queue[1], (PieceType)p1.queue[2], (PieceType)p1.queue[3], (PieceType)p1.queue[4]};
        game.p1_game.hold = p1.hold == 7 ? std::nullopt : std::optional(Piece((PieceType)p1.hold));

        game.p2_game.board = p2.b;
        game.p2_game.current_piece = Piece((PieceType)p2.p_type, Coord(p2.p_x, p2.p_y), (RotationDirection)p2.p_rot, spinType::null);
        game.p2_game.garbage_meter = p2.meter;
        game.p2_game.queue = {(PieceType)p2.queue[0], (PieceType)p2.queue[1], (PieceType)p2.queue[2], (PieceType)p2.queue[3], (PieceType)p2.queue[4]};
        game.p2_game.hold = p2.hold == 7 ? std::nullopt : std::optional(Piece((PieceType)p2.hold));
    }

   public:
    inline bool update(Shakkar::inputs& inputs) {
        auto filename = inputs.getDroppedFile();

        if (!filename.empty()) {
            file.open(filename, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Could not open file" << std::endl;
                return false;
            }
            file_loaded = true;
            // calc file size, and then put the cursor back to the beginning of the file
            file.seekg(0, std::ios::end);
            file_size = file.tellg();
            if (file_size == -1) {
                std::cerr << "Could not get file size" << std::endl;
                return false;
            }
            file_size = std::max(0, (int)file_size);
            file.seekg(0, std::ios::beg);
            parse_data(false);
        }

        if (inputs.getKey(SDLK_LEFT).pressed) {
            parse_data(true);
        }
        if (inputs.getKey(SDLK_RIGHT).pressed) {
            parse_data(false);
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

    void draw_hold_and_queue(Window& window, Game& player, Rect& area) {
        {
            FPoint cell_size = {area.w / 14.0f, area.h / 20.0f};
            Rect hold = {
                int(area.x),
                int(area.y + cell_size.y),
                int(2 * cell_size.x),
                int(2 * cell_size.y)};

            draw_stasis_piece(window, hold, player.hold);

            Rect queue = {
                int(area.x + 12 * cell_size.x),
                int(ceil(area.y + 2 * cell_size.y)),
                int(2 * cell_size.x),
                int(ceil(2 * 5 * cell_size.y))};

            for (int i = 0; i < 5; i++) {
                std::optional<Piece> queue_piece = Piece(player.queue[i]);
                draw_stasis_piece(window, {queue.x, int(ceil(queue.y + i * 2 * cell_size.y)), queue.w, int(ceil(2 * cell_size.y))}, queue_piece);
            }
        }
    }

   public:
    inline void render(Window& window) {
        window.getWindowSize(windowWidth, windowHeight);

        window.clear();

        window.push_color(255, 0, 0, 255);

        {
            // draw player 1
            Rect p1_zone = {int(windowWidth * 0.01), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98)};
            window.drawRectFilled(p1_zone);
            Rect p1_general_area = window.getInnerRect(p1_zone, (14.0f / 21.0f));

            Rect p1_name_area = {p1_general_area.x, p1_general_area.y + p1_general_area.h - (p1_general_area.h / 21), p1_general_area.w, p1_general_area.h / 21};
            window.setDrawColor(0, 0, 0, 255);
            window.drawRectFilled(p1_name_area);
            window.setDrawColor(255, 255, 255, 255);
            window.drawText("player 1", p1_name_area);

            Rect p1_game_area = window.getInnerRect(p1_general_area, board_aspect_ratio);
            p1_game_area.y = p1_general_area.y;
            draw_board_background(window, p1_game_area);
            if(game.p1_game.current_piece.type != PieceType::Empty)
                draw_piece(window, p1_game_area, game.p1_game.current_piece);
            draw_board(window, p1_game_area, game.p1_game.board);
            draw_hold_and_queue(window, game.p1_game, p1_game_area);
        }
        {
            // draw player 2

            window.setDrawColor(60, 50, 200, 255);
            Rect p2 = {int(windowWidth * 0.51), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98)};
            window.drawRectFilled(p2);
            Rect p2_general_area = window.getInnerRect(p2, (14.0f / 21.0f));
            Rect p2_name_area = {p2_general_area.x, p2_general_area.y + p2_general_area.h - (p2_general_area.h / 21), p2_general_area.w, p2_general_area.h / 21};
            window.setDrawColor(0, 0, 0, 255);
            window.drawRectFilled(p2_name_area);
            window.setDrawColor(255, 255, 255, 255);
            window.drawText("player 2", p2_name_area);

            Rect p2_game_area = window.getInnerRect(p2_general_area, board_aspect_ratio);
            p2_game_area.y = p2_general_area.y;
            draw_board_background(window, p2_game_area);
            if (game.p2_game.current_piece.type != PieceType::Empty)
                draw_piece(window, p2_game_area, game.p2_game.current_piece);
            draw_board(window, p2_game_area, game.p2_game.board);
            draw_hold_and_queue(window, game.p2_game, p2_game_area);
        }

        window.pop_color();
        window.display();
    }
};
