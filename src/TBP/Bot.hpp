#pragma once

#include <cstdio>
#include <fstream>

#ifdef __linux__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

class Bot {
        inline void start(const char* path) {
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

            execl(path, NULL);
            perror("execl");
            exit(1);
        } else {
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
#endif
    }

    inline void send(std::string& message) {
#ifdef __linux__
        fprintf(to_child, "%s\n", message.c_str());
        fflush(to_child);
#endif
    }

    inline std::string receive() {
#ifdef __linux__
        std::string message;
        char buffer[1024];
        fgets(buffer, 1024, from_child);
        message = buffer;
        return message;
#endif
    }

    inline void stop() {
#ifdef __linux__
        fclose(to_child);
        fclose(from_child);
#endif
    }

#ifdef __linux__
    int parent_to_child[2]{};
    int child_to_parent[2]{};

    FILE* to_child{};
    FILE* from_child{};
#endif
   public:
    bool running = false;
};