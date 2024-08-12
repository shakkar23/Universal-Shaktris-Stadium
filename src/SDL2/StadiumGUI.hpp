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
class StadiumGUI {
private:
    VersusGame game;
    enum class GameState {
        IDLE,
        PLAYING,
        SETUP,
        GAME_OVER,
    } game_state = GameState::IDLE;

    // pieces per second that the bots will play at
    float pps = 5;
    // the frame count for the bots to determine when they should move
    int frameCount = 0;
    // recorded stats
    std::array<int, 2> num_wins = { 0, 0 };
    int num_games = 0;
    int num_draws = 0;
    // player interfaces for the bots
    Bot player_1;
    Bot player_2;

    int windowWidth{}, windowHeight{};

public:
    inline bool update(Shakkar::inputs& inputs) {
        auto restart_bot_game = [](Bot& bot, Game& game, Game& opp) {
            std::vector<PieceType> tbp_queue(Game::queue_size + 1);
            tbp_queue[0] = game.current_piece.type;
            for (int i = 0; i < Game::queue_size; i++) {
                tbp_queue[i + 1] = game.queue[i];
            }
            bot.TBP_start(opp, game.board, tbp_queue, game.hold, game.stats.b2b != 0, game.stats.combo);
        };
        switch (game_state) {
        case GameState::IDLE: {
            if (inputs.getKey(SDLK_RETURN).pressed) {
                if (player_1.is_running() && player_2.is_running()) {

                    game_state = GameState::SETUP;
                }
            }
            std::string file = inputs.getDroppedFile();
            if (!file.empty()) {
                if (inputs.getMouse().x < windowWidth / 2) {
                    player_1.start(file.c_str());
                }
                else {
                    player_2.start(file.c_str());
                }
            }
        }break;
        
        case GameState::PLAYING: {
            if (game.game_over) {
                game_state = GameState::GAME_OVER;
            }

            // if need to move then ask the bots for moves
            if (frameCount >= UPDATES_PER_SECOND / pps) {
                player_1.TBP_suggest();
                Piece suggestion_1 = player_1.TBP_suggestion()[0];

                player_2.TBP_suggest();
                Piece suggestion_2 = player_2.TBP_suggestion()[0];

                game.p1_move.null_move = false;
                game.p1_move.piece = suggestion_1;

                bool p1_first_hold = false;
                if (!game.p1_game.hold && suggestion_1.type != game.p1_game.current_piece.type) {
                    p1_first_hold = true;
                }

                bool p2_first_hold = false;
                if (!game.p2_game.hold && suggestion_2.type != game.p2_game.current_piece.type) {
                    p2_first_hold = true;
                }

                game.p2_move.null_move = false;
                game.p2_move.piece = suggestion_2;

                game.play_moves();

                bool p2_play = false;
                if (game.p2_accepts_garbage)
                {
                    restart_bot_game(player_2, game.p2_game, game.p1_game);
                }
                else {
                    if (p2_first_hold)
                        player_2.TBP_new_piece(game.p2_game.queue[3]);
                    player_2.TBP_new_piece(game.p2_game.queue.back());
                    p2_play = true;
                }




                bool p1_play = false;
                if (game.p1_accepts_garbage)
                {
                    restart_bot_game(player_1, game.p1_game, game.p2_game);
                }
                else {
                    if (p1_first_hold)
                        player_1.TBP_new_piece(game.p1_game.queue[3]);
                    player_1.TBP_new_piece(game.p1_game.queue.back());

                    p1_play = true;
                }

                
                if(p2_play)
                    player_2.TBP_play(game.p1_game, suggestion_2);

                if(p1_play)
                    player_1.TBP_play(game.p2_game, suggestion_1);

                frameCount = 0;
            }

            frameCount++;
        } break;
        
        case GameState::SETUP: {
            game = VersusGame();

            restart_bot_game(player_1, game.p1_game, game.p2_game);

            restart_bot_game(player_2, game.p2_game, game.p1_game);
            frameCount = 0;

            game_state = GameState::PLAYING;
        } break;
        
        case GameState::GAME_OVER: {
            // save stats about the game here
            if(game.state == VersusGame::State::P1_WIN)
			{
				num_wins[0]++;
			}
			else if(game.state == VersusGame::State::P2_WIN)
			{
				num_wins[1]++;
			}
			else if(game.state == VersusGame::State::DRAW)
			{
				num_draws++;
			}
            num_games++;
            std::cout.setstate(std::ios_base::goodbit);
            std::cout.clear();
            // clear console
            std::cout << "\033[2J\033[1;1H";
            std::cout << 
                "Player 1 wins: " << num_wins[0] << 
                "\nPlayer 2 wins: " << num_wins[1] << 
                "\nDraws: " << num_draws << 
                "\nTotal games: " << num_games << std::endl;
            std::cout.setstate(std::ios_base::failbit);


			game_state = GameState::SETUP;
		} break;
        
        } // end switch

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
            Rect p1_zone = { int(windowWidth * 0.01), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98) };
            window.drawRectFilled(p1_zone);
            Rect p1_general_area = window.getInnerRect(p1_zone, (14.0f / 21.0f));

            Rect p1_name_area = { p1_general_area.x, p1_general_area.y + p1_general_area.h - (p1_general_area.h / 21), p1_general_area.w, p1_general_area.h / 21 };
            window.setDrawColor(0, 0, 0, 255);
            window.drawRectFilled(p1_name_area);
            window.setDrawColor(255, 255, 255, 255);
            window.drawText(player_1.get_name().empty() ? "NONE" : player_1.get_name(), p1_name_area);

            Rect p1_game_area = window.getInnerRect(p1_general_area, board_aspect_ratio);
            p1_game_area.y = p1_general_area.y;
            draw_board_background(window, p1_game_area);
            draw_piece(window, p1_game_area, game.p1_game.current_piece);
            draw_board(window, p1_game_area, game.p1_game.board);
            draw_hold_and_queue(window, game.p1_game, p1_game_area);
        }
        {
            // draw player 2

            window.setDrawColor(60, 50, 200, 255);
            Rect p2 = { int(windowWidth * 0.51), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98) };
            window.drawRectFilled(p2);
            Rect p2_general_area = window.getInnerRect(p2, (14.0f / 21.0f));
            Rect p2_name_area = { p2_general_area.x, p2_general_area.y + p2_general_area.h - (p2_general_area.h / 21), p2_general_area.w, p2_general_area.h / 21 };
            window.setDrawColor(0, 0, 0, 255);
            window.drawRectFilled(p2_name_area);
            window.setDrawColor(255, 255, 255, 255);
            window.drawText(player_2.get_name().empty() ? "NONE" : player_2.get_name(), p2_name_area);

            Rect p2_game_area = window.getInnerRect(p2_general_area, board_aspect_ratio);
            p2_game_area.y = p2_general_area.y;
            draw_board_background(window, p2_game_area);
            draw_piece(window, p2_game_area, game.p2_game.current_piece);
            draw_board(window, p2_game_area, game.p2_game.board);
            draw_hold_and_queue(window, game.p2_game, p2_game_area);
        }

        window.pop_color();
        window.display();
    }
};