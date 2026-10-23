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
    bool should_skip_suggest()const;
    
    // threads goes to bots that advertise set_threads; 0 leaves the bot to choose.
    // garbage_delay, the turns an attack spends in transit, goes to every bot in rules
    void start(const char* path, long long num_nodes, int threads = 0, int garbage_delay = 0);
    void stop();
    const std::string& get_name() const;
    const std::string& get_author() const;
    const std::string& get_version() const;

    void TBP_play(const Game& ours, const Game& opp, const Piece& piece);

    nlohmann::json TBP_info();

    void TBP_suggest();

    std::vector<Piece> TBP_suggestion();

    // with rng, the start carries the seven-bag state: the pieces left in the bag after the queue
    void TBP_start(const Game& ours, const Game& opp, const RNG* rng = nullptr);

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

public:
    std::string name;
private:
    std::string author;
    std::string version;
    bool stateless_bot_protocol = false;
    bool skip_suggest = false;
    bool set_min_nodes = false;
    bool set_threads = false;
    int num_threads = 0;
    bool running = false;
};