#include "DrawStyleEdge.h"


void DrawStyleEdge(SDL_Renderer *r, const SDL_Rect &rect,
                   SDL_Color highlight, SDL_Color shadow,
                   bool topLeftHi, bool bottomRightHi) {

    int x1 = rect.x, y1 = rect.y;
    int x2 = rect.x + rect.w - 1, y2 = rect.y + rect.h - 1;

    auto &tl = topLeftHi ? highlight : shadow;
    SDL_SetRenderDrawColor(r, tl.r, tl.g, tl.b, 255);
    SDL_RenderDrawLine(r, x1, y2, x1, y1);      // left
    SDL_RenderDrawLine(r, x1, y1, x2 - 1, y1);  // top: stops 1px before right

    auto &br = bottomRightHi ? highlight : shadow;
    SDL_SetRenderDrawColor(r, br.r, br.g, br.b, 255);
    SDL_RenderDrawLine(r, x2, y1, x2, y2);      // right
    SDL_RenderDrawLine(r, x2, y2, x1 + 1, y2);  // bottom: stops 1px before left
}


void DrawBevelRaised(SDL_Renderer *r, const SDL_Rect &rect,
                     SDL_Color highlight, SDL_Color shadow) {

    DrawStyleEdge(r, rect, highlight, shadow, true, false);
}


void DrawBevelLowered(SDL_Renderer *r, const SDL_Rect &rect,
                      SDL_Color highlight, SDL_Color shadow) {

    DrawStyleEdge(r, rect, highlight, shadow, false, true);
}
