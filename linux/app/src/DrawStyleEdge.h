#pragma once

#include <SDL.h>


void DrawStyleEdge(SDL_Renderer *r, const SDL_Rect &rect,
                   SDL_Color highlight, SDL_Color shadow,
                   bool topLeftHighlight, bool bottomRightHighlight);


void DrawBevelRaised(SDL_Renderer *r, const SDL_Rect &rect,
                     SDL_Color highlight, SDL_Color shadow);


void DrawBevelLowered(SDL_Renderer *r, const SDL_Rect &rect,
                      SDL_Color highlight, SDL_Color shadow);
