#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <string>


namespace TSeBitmapObject {

    // TSeBitmapObject.DrawRect
    void DrawRect(SDL_Renderer *r, SDL_Texture *atlas,
                  const SDL_Rect &src, const SDL_Rect &dst,
                  const std::string &tileStyle = "tsStretch");

    // TSeBitmapObject.DrawNormal
    void DrawNormal(SDL_Renderer *r, SDL_Texture *tex,
                    const SDL_Rect &dst,
                    const std::string &tileStyle = "tsStretch");

    // TSeBitmapObject.Draw (9-patch)
    void Draw(SDL_Renderer *r, SDL_Texture *atlas,
              const SDL_Rect &src, int ml, int mt, int mr, int mb,
              const SDL_Rect &dst);

    // same but with a pre-cropped texture instead of atlas rect
    void DrawTex(SDL_Renderer *r, SDL_Texture *tex,
                 int ml, int mt, int mr, int mb,
                 const SDL_Rect &dst, bool skipCenter = false);

    // TSeStyleObject.DrawObjectText
    void DrawText(SDL_Renderer *r, TTF_Font *font, SDL_Color color,
                  const std::string &text, const SDL_Rect &rect,
                  const std::string &align = "taLeft");
}
