#pragma once
#include "Window.hpp"
#include "inputs.hpp"

#include <algorithm>

using Rect = SDL_Rect;




class Stadium {
public:

	inline bool update(Shakkar::inputs &inputs) {
		return true;
	}

	inline Rect getInnerRect(Rect parent, float aspect_ratio) {
		int height, width;
		if ((float)parent.w / parent.h > aspect_ratio)
		{
			height = std::min(parent.h, int(parent.w / aspect_ratio));
			width = int(height * aspect_ratio);
		}
		else
		{
			width = std::min(parent.w, int(parent.h * aspect_ratio));
			height = int(width / aspect_ratio);
		}

		Rect board = { parent.x + (parent.w - width) / 2, parent.y + (parent.h - height) / 2, width, height };

		return board;

	}

	inline void render(Window& window) {
		int windowWidth{}, windowHeight{};
		window.getWindowSize(windowWidth, windowHeight);
		window.clear();
		uint8_t r, g, b, a;
		window.getDrawColor(r,g,b,a);



		window.setDrawColor(200, 50, 60, 255);
		Rect p1 = { int(windowWidth * 0.01), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98) };
		window.drawRectFilled(p1);
		Rect board = getInnerRect(p1, 12. / 20.);

		window.setDrawColor(0, 255, 0, 255);
		window.drawRectFilled(board);

		window.setDrawColor(60, 50, 200, 255);
		Rect p2 = { int(windowWidth * 0.51), int(windowHeight * 0.01), int(windowWidth * 0.48), int(windowHeight * .98) };
		window.drawRectFilled(p2);
		Rect board2 = getInnerRect(p2, 12. / 20.);
		window.setDrawColor(0, 255, 0, 255);
		window.drawRectFilled(board2);






		window.setDrawColor(r, g, b, a);
		window.display();
	}

};