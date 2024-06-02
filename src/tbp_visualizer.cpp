#include <iostream>
#include <unordered_map>

#include "SDL2/SDL.h"
#include "VisualizerGUI.hpp"
#include "VersusGame.hpp"
#include "Window.hpp"
#include "inputs.hpp"

void ProcessInputs(SDL_Event& event, bool& shouldDisplay, bool& windowSizedChanged, Shakkar::inputs& input, bool& gameRunning);

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);
    TTF_Init();
    struct SDL_Cleanup {
        ~SDL_Cleanup() {
            SDL_Quit();
            TTF_Quit();
        }
    } sdl_quit;
    Window window("Visualizer", 400, 600);

    VisualizerGUI game;

    Shakkar::inputs input;

    bool gameRunning = true;
    SDL_Event event;

    // rendering constantly, and not actually displaying causes memory to stack higher and higher until the frames can be shown
    bool shouldDisplay = false;
    bool windowSizedChanged = false;

    float alpha = 0.0;
    Uint64 last_time = SDL_GetPerformanceCounter();
    Uint64 ticks = 0;

    while (gameRunning) {
        ProcessInputs(event, shouldDisplay, windowSizedChanged, input, gameRunning);

        // skip frames that cant be shown due to window not currently accepting frames to display
        if (!shouldDisplay) {
            const auto now = SDL_GetPerformanceCounter();
            alpha += (double)((double)(now - last_time) / SDL_GetPerformanceFrequency() * UPDATES_PER_SECOND);
            last_time = now;

            while (alpha > 1.0) {
                if (!game.update(input)) {
                    gameRunning = false;
                    break;
                }
                input.update();

                alpha -= 1.0;
            }
            if (gameRunning) {
                game.render(window);
            }
            else {
                gameRunning = false;
            }
        }
    }

    return 0;
}

void ProcessInputs(SDL_Event& event, bool& shouldDisplay, bool& windowSizedChanged, Shakkar::inputs& input, bool& gameRunning) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_WINDOWEVENT: {
            switch (event.window.event) {
            case SDL_WINDOWEVENT_MINIMIZED:
                shouldDisplay = false;
                break;
            case SDL_WINDOWEVENT_MAXIMIZED:
                shouldDisplay = false;
                break;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                windowSizedChanged = true;
                break;
            default:
                break;
            }
        } break;
        case SDL_DROPFILE: {
            input.updateDroppedFile(event.drop.file);
            SDL_free(event.drop.file);
        } break;
        default: {
            if ((event.key.type == SDL_KEYDOWN) || (event.key.type == SDL_KEYUP)) {
                if (event.key.state == SDL_PRESSED) {
                    if (event.key.repeat)
                        continue;
                    input.addKey(event.key.keysym.sym);
                    // std::cout << "Physical" << SDL_GetScancodeName(event.key.keysym.scancode) << " key acting as the " << SDL_GetKeyName(event.key.keysym.sym) << " key" << std::endl;
                }
                else {
                    input.removeKey(event.key.keysym.sym);
                }
            }
            // mouse events
            else if (event.type == SDL_MOUSEBUTTONDOWN) {
                input.updateMouseButtons(event.button.button, true);
            }
            else if (event.type == SDL_MOUSEBUTTONUP) {
                input.updateMouseButtons(event.button.button, false);
            }
            else if (event.type == SDL_MOUSEMOTION) {
                input.updateMousePos(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
            }
            else if (event.type == SDL_MOUSEWHEEL) {
                input.updateMouseWheel(event.wheel.x, event.wheel.y, event.wheel.direction);
            }
            else if (event.type == SDL_QUIT)  // X button on window, or something else that closes the window from the OS
                gameRunning = false;
        } break;
        }
    }
}
