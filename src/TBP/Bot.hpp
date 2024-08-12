#pragma once

#include <optional>
#include <vector>

#include "Board.hpp"
#include "Game.hpp"
#include "Piece.hpp"
#include "json.hpp"

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#elif _WIN32
#include <windows.h>
#undef min
#undef max
#endif

class Bot {
public:

    bool is_running() const;
    
    void start(const char* path);
    void stop();
    const std::string& get_name() const;
    const std::string& get_author() const;
    const std::string& get_version() const;

    void TBP_play(const Game &opp, const Piece& move);

    nlohmann::json TBP_info();

    void TBP_suggest();

    std::vector<Piece> TBP_suggestion();

    void TBP_start(const Game& opp, const Board& board, const std::vector<PieceType>& queue, std::optional<Piece> hold = std::nullopt, bool back_to_back = false, int combo = 0);

    void TBP_new_piece(PieceType t);

    // stops the game itself, a new game CAN be started by sending a start command
    void TBP_stop();

private:
    // if this is sent, the game will end and the bot will be disconnected
    void TBP_quit();

private:
    void send(std::string message);
    std::string receive();

#ifdef __linux__
    int parent_to_child[2]{};
    int child_to_parent[2]{};

    FILE* to_child{};
    FILE* from_child{};
#elif _WIN32
    HANDLE g_hChildStd_IN_Rd = NULL;
    HANDLE g_hChildStd_IN_Wr = NULL;
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;
#endif


    std::string name;
    std::string author;
    std::string version;
    bool running = false;
};