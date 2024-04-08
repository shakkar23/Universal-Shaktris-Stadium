#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

class Window {
public:
    Window(const char* p_title, int p_w, int p_h);
    ~Window();
    // window related
    int getRefreshrate();
    void getWindowSize(int& x, int& y);
    void setWindowSize(int x, int y);
    // drawing related
    bool render(SDL_Rect src, SDL_Rect dst, SDL_Texture* tex);
    void renderCopy(SDL_Texture* texture,
        const SDL_Rect* srcrect,
        const SDL_Rect* dstrect);
    void display();
public:
    void clear();
    void drawCircle(int x, int y, int r);
    void drawRect(SDL_Rect rec);
    void drawRectFilled(SDL_Rect rec);
    // utility
    SDL_Texture* CreateTextureFromSurface(SDL_Surface* surface);

    void setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);

    void getDrawColor(Uint8& r, Uint8& g, Uint8& b, Uint8& a);

private:
    SDL_Window* window;
    SDL_Renderer* renderer;

    SDL_Renderer* getRenderer();
};

