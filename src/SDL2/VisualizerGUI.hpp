#pragma once

#include <algorithm>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <optional>

#include "Bot.hpp"
#include "VersusGame.hpp"
#include "Window.hpp"
#include "inputs.hpp"

using Rect = SDL_Rect;

using Point = SDL_Point;
using FPoint = SDL_FPoint;
using Color = SDL_Color;

constexpr float board_aspect_ratio = 14. / 20.;

class VisualizerGUI {
private:
    Game game;
    int windowWidth{}, windowHeight{};

public:
    inline bool update(Shakkar::inputs& inputs) {
        auto ctrl =  inputs.getKey(SDLK_LCTRL);
        auto v = inputs.getKey(SDLK_v);
        
        if (ctrl.held && v.pressed) {
		    // get the clipboard
            std::string clipboard = SDL_GetClipboardText();

            // parse the clipboard into json
            nlohmann::json j = nlohmann::json::parse(clipboard);
            // example json:
            /*
{"back_to_back":true,"board":[["G","G",null,"G","G","G","G","G","G","G"],
["G",null,null,null,"G","G","G","G","G",null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null],
[null,null,null,null,null,null,null,null,null,null]],
"combo":1,"hold":"J","queue":["Z","T","S","L","I","O"],
"type":"start"}
            */

            // get the board
            auto json_board = j["board"];
            // taken from https://github.com/Nightcrab/nana/blob/8f5cbb6e6c559b1418c28749166ac2f6713dfdbf/cpp/tbp.cpp#L90
            auto json_to_board = [](nlohmann::json json) -> Board{
                Board board;
                for (int y = 0; y < Board::height; ++y) {
                    for (int x = 0; x < Board::width; ++x) {
                        if (!json[y][x].is_null()) {
                            board.set(x, y);
                        }
                    }
                }
                return board;
            };

            game.board = json_to_board(json_board);

            // get the queue

            auto json_queue = j["queue"];
            std::vector<PieceType> queue;
            // taken from the bot.cpp file
            auto str_to_piecetype = [](std::string type) -> PieceType {
                if (type == "S")
                    return PieceType::S;
                else if (type == "Z")
                    return PieceType::Z;
                else if (type == "J")
                    return PieceType::J;
                else if (type == "L")
                    return PieceType::L;
                else if (type == "T")
                    return PieceType::T;
                else if (type == "O")
                    return PieceType::O;
                else if (type == "I")
                    return PieceType::I;
                throw std::runtime_error("invalid piece");
                };

            for (auto& piece : json_queue) {
				queue.push_back(str_to_piecetype(piece.get<std::string>()));
			}

            for(int i = 0; i < game.queue.size(); i++) {
				game.queue[i] = queue[i];
            }
			
            // get the hold
			auto json_hold = j["hold"];
			if (json_hold.is_null()) {
				game.hold = std::nullopt;
			}
			else {
				game.hold = str_to_piecetype(json_hold.get<std::string>());
			}

        }
        return true;
    }

private:
    inline Color get_color(PieceType type) {
        switch (type) {
        case PieceType::I:
            return { 0, 255, 255, 255 };
        case PieceType::J:
            return { 0, 0, 255, 255 };
        case PieceType::L:
            return { 255, 165, 0, 255 };
        case PieceType::O:
            return { 255, 255, 0, 255 };
        case PieceType::S:
            return { 0, 255, 0, 255 };
        case PieceType::T:
            return { 128, 0, 128, 255 };
        case PieceType::Z:
            return { 255, 0, 0, 255 };
        default:
            return { 0, 0, 0, 0 };
        }
    }

    inline void draw_board_background(Window& window, Rect area) {
        // first two columns are for rendering the hold
        // the next 10 columns are for the board
        // the last two columns are for the queue

        FPoint cell_size = { area.w / 14.0f, area.h / 20.0f };

        // draw filled board
        {
            window.setDrawColor(0, 0, 0, 255);
            Rect board = {
                int(area.x + 2 * cell_size.x),
                int(area.y),
                int(10 * cell_size.x),
                int(20 * cell_size.y) };
            window.drawRectFilled(board);
        }

        window.setDrawColor(255, 255, 255, 255);
        for (int i = 2; i < 12; i++) {
            for (int j = 0; j < 20; j++) {
                Rect cell = {
                    int(area.x + i * cell_size.x),
                    int(area.y + j * cell_size.y),
                    int(ceil(cell_size.x)),
                    int(ceil(cell_size.y)) };
                window.drawRect(cell);
            }
        }
    }
    inline void draw_stasis_piece(Window& window, Rect area, std::optional<Piece> piece) {
        // assume that the area is the hold area and we render the piece with (0, 0) as the center, the grid being a 5x5 grid
        FPoint cell_size = { area.w / 5.0f, area.h / 5.0f };

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
                (int)ceil(cell_size.x), (int)ceil(cell_size.y) };

            window.drawRectFilled(cell);
        }
    }

    inline void draw_board(Window& window, Rect board, Board& b) {
        FPoint cell_size = { board.w / 14.0f, board.h / 20.0f };
        Point board_point = { int(board.x + cell_size.x * 2), int(board.y + board.h) };

        window.setDrawColor(100, 100, 100, 255);

        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 20; j++) {
                if (b.get(i, j) != 0) {
                    Rect cell = { int(ceil(board_point.x + i * cell_size.x)), int(ceil(board_point.y - (j + 1) * cell_size.y)), (int)ceil(cell_size.x), (int)ceil(cell_size.y) };
                    window.drawRectFilled(cell);
                }
            }
        }
    }

    void draw_piece(Window& window, Rect area, Piece piece) {
        FPoint cell_size = { area.w / 14.0f, area.h / 20.0f };
        Rect board = {
            int(area.x + 2.0f * cell_size.x),
            int(area.y),
            int(10.0f * cell_size.x),
            int(20.0f * cell_size.y) };
        Point board_point = { board.x, board.y + board.h };
        Color col = get_color(piece.type);
        window.setDrawColor(col.r, col.g, col.b, col.a);

        for (auto& mino : piece.minos) {
            Rect cell = {
                int(ceil(board_point.x + (piece.position.x + mino.x) * cell_size.x)),
                int(ceil(board_point.y - (piece.position.y + mino.y) * cell_size.y)),
                (int)ceil(cell_size.x), (int)ceil(cell_size.y) };

            window.drawRectFilled(cell);
        }
    }

    void draw_hold_and_queue(Window& window, Game& player, Rect& area) {
        {
            FPoint cell_size = { area.w / 14.0f, area.h / 20.0f };
            Rect hold = {
                int(area.x),
                int(area.y + cell_size.y),
                int(2 * cell_size.x),
                int(2 * cell_size.y) };

            draw_stasis_piece(window, hold, player.hold);

            Rect queue = {
                int(area.x + 12 * cell_size.x),
                int(ceil(area.y + 2 * cell_size.y)),
                int(2 * cell_size.x),
                int(ceil(2 * 5 * cell_size.y)) };

            for (int i = 0; i < 5; i++) {
                std::optional<Piece> queue_piece = Piece(player.queue[i]);
                draw_stasis_piece(window, { queue.x, int(ceil(queue.y + i * 2 * cell_size.y)), queue.w, int(ceil(2 * cell_size.y)) }, queue_piece);
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
            Rect p1_zone = { int(windowWidth * 0.01), int(windowHeight * 0.01), int(windowWidth * 0.98), int(windowHeight * 0.98) };
            window.drawRectFilled(p1_zone);
            Rect p1_general_area = window.getInnerRect(p1_zone, (14.0f / 21.0f));

            Rect p1_name_area = { p1_general_area.x, p1_general_area.y + p1_general_area.h - (p1_general_area.h / 21), p1_general_area.w, p1_general_area.h / 21 };
            window.setDrawColor(0, 0, 0, 255);
            window.drawRectFilled(p1_name_area);
            window.setDrawColor(255, 255, 255, 255);
            window.drawText("None", p1_name_area);

            Rect p1_game_area = window.getInnerRect(p1_general_area, board_aspect_ratio);
            p1_game_area.y = p1_general_area.y;
            draw_board_background(window, p1_game_area);
            draw_piece(window, p1_game_area, game.current_piece);
            draw_board(window, p1_game_area, game.board);
            draw_hold_and_queue(window, game, p1_game_area);
        }
      

        window.pop_color();
        window.display();
    }
};
