
#include "Bot.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

bool Bot::is_running() const {
    return running;
}

void Bot::start(const char* path) {
#ifdef __linux__
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // child
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        dup2(parent_to_child[0], STDIN_FILENO);
        dup2(child_to_parent[1], STDOUT_FILENO);

        execl(path, "", NULL);
        perror("execl");
        exit(1);
    }
    else {
        // parent
        close(parent_to_child[0]);
        close(child_to_parent[1]);

        to_child = fdopen(parent_to_child[1], "w");
        from_child = fdopen(child_to_parent[0], "r");

        if (to_child == NULL || from_child == NULL) {
            perror("fdopen");
            exit(1);
        }
    }
#elif _WIN32
    SECURITY_ATTRIBUTES saAttr{};

    // Set the bInheritHandle flag so pipe handles are inherited. 
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT. 

    CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0);

    // Ensure the read handle to the pipe for STDOUT is not inherited.

    SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    // Create a pipe for the child process's STDIN. 

    CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0);

    // Ensure the write handle to the pipe for STDIN is not inherited. 

    SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);
    STARTUPINFO startup{};
    startup.cb = sizeof(STARTUPINFO);
    startup.lpReserved = nullptr;
    startup.lpDesktop = nullptr;
    startup.lpTitle = nullptr;
    startup.cbReserved2 = 0;
    startup.lpReserved2 = nullptr;
    startup.hStdError = g_hChildStd_OUT_Wr;
    startup.hStdOutput = g_hChildStd_OUT_Wr;
    startup.hStdInput = g_hChildStd_IN_Rd;
    startup.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION procInfo{};

    BOOL worked = CreateProcess(nullptr, const_cast<char*>(path), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &startup, &procInfo);

#endif

    auto info = TBP_info();

    author = info["author"].get<std::string>();
    name = info["name"].get<std::string>();
    version = info["version"].get<std::string>();

    running = true;
}

void Bot::send(std::string message) {
#ifdef __linux__
    fprintf(to_child, "%s\n", message.c_str());
    fflush(to_child);
#elif _WIN32
    DWORD dwWritten;
    std::string chBuf = message + "\n";
    BOOL bSuccess = FALSE;

    bSuccess = WriteFile(g_hChildStd_IN_Wr, (LPVOID*)chBuf.c_str(),
        chBuf.size(), &dwWritten, NULL);
    if (!bSuccess) return;
#endif
}

std::string Bot::receive() {
#ifdef __linux__
    std::string message;
    char buffer[4096]{};
    std::string temp;
    fgets(buffer, sizeof(buffer), from_child);
    message = buffer;
    return message;
#elif _WIN32
    DWORD dwRead, dwWritten;
    CHAR chBuf[4096]{};
    std::string jsnBuf;
    BOOL bSuccess = FALSE;

    bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf,
        4096, &dwRead, NULL);
    if (!bSuccess || dwRead == 0) return "";

    for (int i = 0; i < dwRead; ++i)
        jsnBuf.push_back(chBuf[i]);

    return jsnBuf;
#endif
}

void Bot::stop() {
    TBP_quit();
#ifdef __linux__
    fclose(to_child);
    fclose(from_child);
#elif _WIN32
    CloseHandle(g_hChildStd_IN_Wr);
    CloseHandle(g_hChildStd_OUT_Rd);
#endif
}

const std::string& Bot::get_name() const {
    return name;
}

const std::string& Bot::get_author() const {
    return author;
}

const std::string& Bot::get_version() const {
    return version;
}

void Bot::TBP_play(const Piece& piece) {
    nlohmann::json play;
    auto px = piece.position.x;
    auto py = piece.position.y;
    auto pk = piece.type;
    play["type"] = "play";

    play["move"]["location"]["type"] = [](PieceType type) -> std::string {
        switch (type) {
        case PieceType::S:
            return "S";
            break;
        case PieceType::Z:
            return "Z";
            break;
        case PieceType::J:
            return "J";
            break;
        case PieceType::L:
            return "L";
            break;
        case PieceType::T:
            return "T";
            break;
        case PieceType::O:
            return "O";
            break;
        case PieceType::I:
            return "I";
            break;
        default:
            throw std::runtime_error("invalid piece");
            break;
        }
        }(piece.type);
        play["move"]["location"]["orientation"] = [](RotationDirection dir) -> std::string {
            switch (dir) {
            case North:
                return "north";
                break;
            case East:
                return "east";
                break;
            case South:
                return "south";
                break;
            case West:
                return "west";
                break;
            default:
                throw std::runtime_error("invalid direction");
                break;
            }
            }(piece.rotation);
            play["move"]["location"]["x"] = piece.position.x;
            play["move"]["location"]["y"] = piece.position.y;

            play["move"]["spin"] = [](spinType s) -> std::string {
                switch (s) {
                case spinType::null:
                    return "none";
                    break;
                case spinType::mini:
                    return "mini";
                    break;
                case spinType::normal:
                    return "full";
                    break;
                default:
                    throw std::runtime_error("invalid spin");
                    break;
                }
                }(piece.spin);

                send(play.dump());
                std::cout << "TBP play: " << play << std::endl
                    << std::endl;
}

nlohmann::json Bot::TBP_info() {
    nlohmann::json uselessInfo;
    uselessInfo = nlohmann::json::parse(receive());
    // cold clear is supposed to be sending this first
    // {"type":"info","name":"Cold Clear","version":"2020-05-05","author":"MinusKelvin","features":[]}

    name = uselessInfo["name"];
    author = uselessInfo["author"];
    version = uselessInfo["version"];
    std::cout << "TBP info: " << uselessInfo << std::endl
        << std::endl;
    return uselessInfo;
}

void Bot::TBP_suggest() {
    nlohmann::json suggest;
    suggest["type"] = "suggest";

    send(suggest.dump());
    std::cout << "TBP suggest: " << suggest << std::endl
        << std::endl;
}
/*
    The suggestion message is sent in response to a suggest message.
    It informs the frontend of what moves the bot wishes to make in order of preference.
    The frontend should play the most preferred valid move.
    If no moves are valid, the bot forfeits and the frontend should tell the bot to stop calculation.
    Whether a hold should be performed is inferred from the type of piece to be placed.
*/

std::vector<Piece> Bot::TBP_suggestion() {
    nlohmann::json suggestion;
    std::cout << "TBP suggestion: ";
    // example: {"moves":[{"location":{"orientation":"north","type":"L","x":8,"y":0},"spin":"none"}],"type":"suggestion"}
    std::string suggestion_str = receive();
    suggestion = nlohmann::json::parse(suggestion_str);
    std::cout << suggestion << std::endl
        << std::endl;
    std::vector<Piece> moves;
    for (const auto& move : suggestion["moves"]) {
        PieceType type;
        RotationDirection orientation;
        int x;
        int y;
        spinType spin;
        type = [](std::string type) -> PieceType {
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
            }(move["location"]["type"]);

            orientation = [](std::string orientation) -> RotationDirection {
                if (orientation == "north")
                    return North;
                else if (orientation == "east")
                    return East;
                else if (orientation == "south")
                    return South;
                else if (orientation == "west")
                    return West;
                throw std::runtime_error("invalid orientation");
                }(move["location"]["orientation"]);

                x = move["location"]["x"].get<int>();
                y = move["location"]["y"].get<int>();

                spin = [](std::string spin) -> spinType {
                    if (spin == "none")
                        return spinType::null;
                    else if (spin == "mini")
                        return spinType::mini;
                    else if (spin == "full")
                        return spinType::normal;
                    throw std::runtime_error("invalid spin");
                    }(move["spin"]);
                    Piece piece = type;
                    piece.position = Coord(x, y);
                    piece.spin = spin;

                    for (int i = 0; i < orientation; ++i)
						piece.rotate(TurnDirection::Right);
                    moves.push_back(piece);
    }
    return moves;
}

void Bot::TBP_start(const Board& board, const std::vector<PieceType> &queue, std::optional<Piece> hold, bool back_to_back, int combo) {
    nlohmann::json start;
    start["type"] = "start";

    if (hold.has_value()) {
        start["hold"] = [](PieceType type) -> nlohmann::json {
            switch (type) {
            case PieceType::S:
                return "S";
                break;
            case PieceType::Z:
                return "Z";
                break;
            case PieceType::J:
                return "J";
                break;
            case PieceType::L:
                return "L";
                break;
            case PieceType::T:
                return "T";
                break;
            case PieceType::O:
                return "O";
                break;
            case PieceType::I:
                return "I";
                break;
            case PieceType::Empty:
                return "I";
                break;
            default:
                return nullptr;
                break;
            }
            }(hold.value().type);
    }
    else
        start["hold"] = nullptr;

    std::vector<std::string> tbpQueue;

    for (const auto& piece : queue) {
        switch (piece) {
        case PieceType::S:
            tbpQueue.push_back("S");
            break;
        case PieceType::Z:
            tbpQueue.push_back("Z");
            break;
        case PieceType::I:
            tbpQueue.push_back("I");
            break;
        case PieceType::T:
            tbpQueue.push_back("T");
            break;
        case PieceType::O:
            tbpQueue.push_back("O");
            break;
        case PieceType::J:
            tbpQueue.push_back("J");
            break;
        case PieceType::L:
            tbpQueue.push_back("L");
            break;
        default:
            throw std::runtime_error("invalid piece");
            break;
        }
    }

    start["queue"] = tbpQueue;
    start["combo"] = combo;
    start["back_to_back"] = back_to_back;

    std::array<std::array<std::optional<std::string>, 40>, 10> tbpBoard;

    for (int x = 0; x < Board::width; ++x)
        for (int y = 0; y < Board::visual_height; ++y) {
            if (board.get(x, y))
                tbpBoard[x][y] = "G";
            else
                tbpBoard[x][y] = std::nullopt;
        }

    start["board"] = nlohmann::json::array();

    for (int y = 0; y < Board::height; ++y) {
        nlohmann::json tmp = nlohmann::json::array();
        for (int x = 0; x < Board::width; ++x) {
            if (tbpBoard[x][y].has_value()) {
                tmp.push_back(tbpBoard[x][y].value());
            }
            else {
                tmp.push_back(nullptr);
            }
        }
        start["board"].push_back(tmp);
    }

    for (int y = Board::height; y < 40; ++y) {
        nlohmann::json tmp = nlohmann::json::array();
        for (int x = 0; x < Board::width; ++x) {
            tmp.push_back(nullptr);
        }
        start["board"].push_back(tmp);
    }

    send(start.dump());

    std::cout << "TBP start: " << start << std::endl
        << std::endl;
}

void Bot::TBP_new_piece(PieceType t) {
    nlohmann::json new_piece;
    new_piece["type"] = "new_piece";

    new_piece["piece"] = [](PieceType t) -> std::string {
        switch (t) {
        case PieceType::S:
            return ("S");
            break;
        case PieceType::Z:
            return ("Z");
            break;
        case PieceType::I:
            return ("I");
            break;
        case PieceType::T:
            return ("T");
            break;
        case PieceType::O:
            return ("O");
            break;
        case PieceType::J:
            return ("J");
            break;
        case PieceType::L:
            return ("L");
            break;
        default:
            return ("Error");
            break;
        }
        }(t);

        send(new_piece.dump());
        std::cout << "TBP new piece: " << new_piece << std::endl
            << std::endl;
}

// stops the game itself, a new game CAN be started by sending a start command
void Bot::TBP_stop() {
    nlohmann::json stop;
    stop["type"] = "stop";

    send(stop.dump());
    std::cout << "TBP stop: " << stop << std::endl
        << std::endl;
}

// if this is sent, the game will end and the bot will be disconnected
void Bot::TBP_quit() {
    nlohmann::json quit;
    quit["type"] = "quit";

    send(quit.dump());
    std::cout << "TBP quit: " << quit << std::endl
        << std::endl;
}