#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <stack>
#include <string>

constexpr int UPDATES_PER_SECOND = 60;
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
    void drawText(const std::string& str, const SDL_Rect rec);
    // utility
    SDL_Texture* CreateTextureFromSurface(SDL_Surface* surface);


    void setDrawColor(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void getDrawColor(Uint8& r, Uint8& g, Uint8& b, Uint8& a);

    void push_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    void pop_color();

    SDL_Rect getInnerRect(SDL_Rect parent, float aspect_ratio);

   private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* default_font;
    int current_ptsize;

    SDL_Renderer* getRenderer();

    std::stack<SDL_Color> colors;
};

