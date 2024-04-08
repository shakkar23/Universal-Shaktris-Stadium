#include <iostream>
#include <unordered_map>

#include "VersusGame.hpp"

#include "SDL2/SDL.h"
#include "Window.hpp"
#include "inputs.hpp"
#include "Stadium.hpp"

void ProcessInputs(SDL_Event& event, bool& shouldDisplay, bool& windowSizedChanged, Shakkar::inputs& input, bool& gameRunning);

int main(int argc, char *argv[]) {
	Window window("UTS", 800, 600);

	Stadium game;

	Shakkar::inputs input;

	bool gameRunning = true;
	SDL_Event event;

	// rendering constantly, and not actually displaying causes memory to stack higher and higher until the frames can be shown
	bool shouldDisplay = false;
	bool windowSizedChanged = false;

	double alpha = 0.0;
	Uint64 last_time = SDL_GetPerformanceCounter();
	Uint64 ticks = 0;

	while (gameRunning) {


		ProcessInputs(event, shouldDisplay, windowSizedChanged, input, gameRunning);

		// skip frames that cant be shown due to window not currently accepting frames to display
		if (!shouldDisplay) {

			const auto now = SDL_GetPerformanceCounter();
			alpha += (double)((double)(now - last_time) / SDL_GetPerformanceFrequency() * 60); // 60 fps
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

void ProcessInputs(SDL_Event& event, bool& shouldDisplay, bool& windowSizedChanged, Shakkar::inputs& input, bool& gameRunning)
{
	while (SDL_PollEvent(&event)) {

		if (event.type == SDL_WINDOWEVENT) {
			if (event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
				shouldDisplay = false;
			}
			else if (event.window.event == SDL_WINDOWEVENT_MAXIMIZED) {
				shouldDisplay = false;
			}
			else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
				windowSizedChanged = true;
			}
		}
		else if ((event.key.type == SDL_KEYDOWN) || (event.key.type == SDL_KEYUP)) {
			if (event.key.state == SDL_PRESSED) {
				if (event.key.repeat)
					continue;
				input.addKey(event.key.keysym.sym);
				//std::cout << "Physical" << SDL_GetScancodeName(event.key.keysym.scancode) << " key acting as the " << SDL_GetKeyName(event.key.keysym.sym) << " key" << std::endl;
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
		else if (event.type == SDL_QUIT) // X button on window, or something else that closes the window from the OS
			gameRunning = false;

	}
}
