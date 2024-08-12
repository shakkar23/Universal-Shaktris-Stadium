#pragma once

#include "Botris.hpp"
#include "./../TBP/Bot.hpp"
#include "VisualizerGUI.hpp"
#include "./../TBP/botris_interface.hpp"
#include <vector>

class BotrisVisualizer
{
public:
	bool update(const Shakkar::inputs& input);

	inline void render(Window& window) {
		gui.render(window);
	}



	using Path = std::vector<Movement>;
	Path pathfind(const Board& board, const Piece& piece);

	// purely used for rendering the Game of the local Bot
	VisualizerGUI gui;
	Botris server;
	Bot local_bot;


	// used for pathfinding to keep around the previous board state
	Game game;
	Game opponent_game;
	std::string session_id;
};

