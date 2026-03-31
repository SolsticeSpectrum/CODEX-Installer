#include "TSeBitmapObject.h"
#include <SDL_ttf.h>
#include <algorithm>


void TSeBitmapObject::DrawRect(SDL_Renderer *r, SDL_Texture *atlas,
                               const SDL_Rect &src, const SDL_Rect &dst,
                               const std::string &tileStyle) {

    if (src.w <= 0 || src.h <= 0 || dst.w <= 0 || dst.h <= 0) return;

    if (tileStyle == "tsTile" || tileStyle == "tsHorzCenterTile" || tileStyle == "tsVertCenterTile") {
        for (int ty = 0; ty < dst.h; ty += src.h) {
            for (int tx = 0; tx < dst.w; tx += src.w) {
                int cw = std::min(src.w, dst.w - tx);
                int ch = std::min(src.h, dst.h - ty);

                SDL_Rect s = {src.x, src.y, cw, ch};
                SDL_Rect d = {dst.x + tx, dst.y + ty, cw, ch};

                SDL_RenderCopy(r, atlas, &s, &d);
            }
        }

        return;
    }

    if (tileStyle == "tsCenter") {
        SDL_Rect d = {dst.x + (dst.w - src.w) / 2, dst.y + (dst.h - src.h) / 2, src.w, src.h};
        SDL_Rect s = src;
        SDL_RenderCopy(r, atlas, &s, &d);
        return;
    }

    // tsStretch
    SDL_Rect s = src;
    SDL_Rect d = dst;
    SDL_RenderCopy(r, atlas, &s, &d);
}


void TSeBitmapObject::DrawNormal(SDL_Renderer *r, SDL_Texture *tex,
                                 const SDL_Rect &dst, const std::string &tileStyle) {

    if (!tex || dst.w <= 0 || dst.h <= 0) return;

    int tw, th;
    SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);

    if (tileStyle == "tsTile") {
        for (int ty = 0; ty < dst.h; ty += th) {
            for (int tx = 0; tx < dst.w; tx += tw) {
                int cw = std::min(tw, dst.w - tx);
                int ch = std::min(th, dst.h - ty);

                SDL_Rect s = {0, 0, cw, ch};
                SDL_Rect d = {dst.x + tx, dst.y + ty, cw, ch};

                SDL_RenderCopy(r, tex, &s, &d);
            }
        }

        return;
    }

    if (tileStyle == "tsCenter") {
        SDL_Rect d = {dst.x + (dst.w - tw) / 2, dst.y + (dst.h - th) / 2, tw, th};
        SDL_RenderCopy(r, tex, nullptr, &d);

        return;
    }

    SDL_Rect d = dst;
    SDL_RenderCopy(r, tex, nullptr, &d);
}


void TSeBitmapObject::Draw(SDL_Renderer *r, SDL_Texture *atlas,
                           const SDL_Rect &srcRect, int ml, int mt, int mr, int mb,
                           const SDL_Rect &dst) {

    if (dst.w <= 0 || dst.h <= 0) return;

    if (ml == 0 && mt == 0 && mr == 0 && mb == 0) {
        SDL_Rect s = srcRect, d = dst;
        SDL_RenderCopy(r, atlas, &s, &d);

        return;
    }

    int sw = srcRect.w, sh = srcRect.h, sx = srcRect.x, sy = srcRect.y;
    int dw = dst.w, dh = dst.h, dx = dst.x, dy = dst.y;

    int eml = std::min(ml, dw / 2);
    int emr = std::min(mr, dw - eml);
    int emt = std::min(mt, dh / 2);
    int emb = std::min(mb, dh - emt);

    int scw = sw - ml  - mr;
    int sch = sh - mt  - mb;
    int dcw = dw - eml - emr;
    int dch = dh - emt - emb;

    auto blit = [&](int sx2, int sy2, int sw2, int sh2, int dx2, int dy2, int dw2, int dh2) {
        if (sw2 <= 0 || sh2 <= 0 || dw2 <= 0 || dh2 <= 0) return;
        SDL_Rect s = {sx2, sy2, sw2, sh2};
        SDL_Rect d = {dx2, dy2, dw2, dh2};
        SDL_RenderCopy(r, atlas, &s, &d);
    };

    // corners
    blit(sx,        sy,        ml, mt,  dx,         dy,         eml, emt);
    blit(sx+sw-mr,  sy,        mr, mt,  dx+dw-emr,  dy,         emr, emt);
    blit(sx,        sy+sh-mb,  ml, mb,  dx,         dy+dh-emb,  eml, emb);
    blit(sx+sw-mr,  sy+sh-mb,  mr, mb,  dx+dw-emr,  dy+dh-emb,  emr, emb);

    // borders
    blit(sx+ml,     sy,        scw, mt,  dx+eml,     dy,         dcw, emt);
    blit(sx+ml,     sy+sh-mb,  scw, mb,  dx+eml,     dy+dh-emb,  dcw, emb);
    blit(sx,        sy+mt,     ml,  sch, dx,         dy+emt,     eml, dch);
    blit(sx+sw-mr,  sy+mt,     mr,  sch, dx+dw-emr,  dy+emt,     emr, dch);

    // center
    blit(sx+ml, sy+mt, scw, sch, dx+eml, dy+emt, dcw, dch);
}


void TSeBitmapObject::DrawTex(SDL_Renderer *r, SDL_Texture *tex,
                              int ml, int mt, int mr, int mb,
                              const SDL_Rect &dst, bool skipCenter) {
    if (!tex || dst.w <= 0 || dst.h <= 0) return;

    int tw, th;
    SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);

    if (ml == 0 && mt == 0 && mr == 0 && mb == 0) {
        SDL_Rect d = dst;
        SDL_RenderCopy(r, tex, nullptr, &d);
        return;
    }

    int dw = dst.w, dh = dst.h, dx = dst.x, dy = dst.y;

    int eml = std::min(ml, dw / 2);
    int emr = std::min(mr, dw - eml);
    int emt = std::min(mt, dh / 2);
    int emb = std::min(mb, dh - emt);

    int scw = tw - ml - mr;
    int sch = th - mt - mb;
    int dcw = dw - eml - emr;
    int dch = dh - emt - emb;

    auto blit = [&](int sx, int sy, int sw, int sh, int dx2, int dy2, int dw2, int dh2) {
        if (sw <= 0 || sh <= 0 || dw2 <= 0 || dh2 <= 0) return;
        SDL_Rect s = {sx, sy, sw, sh};
        SDL_Rect d = {dx2, dy2, dw2, dh2};
        SDL_RenderCopy(r, tex, &s, &d);
    };

    blit(0,       0,       ml, mt,  dx,         dy,         eml, emt);
    blit(tw-mr,   0,       mr, mt,  dx+dw-emr,  dy,         emr, emt);
    blit(0,       th-mb,   ml, mb,  dx,         dy+dh-emb,  eml, emb);
    blit(tw-mr,   th-mb,   mr, mb,  dx+dw-emr,  dy+dh-emb,  emr, emb);

    blit(ml,      0,       scw, mt,  dx+eml,     dy,         dcw, emt);
    blit(ml,      th-mb,   scw, mb,  dx+eml,     dy+dh-emb,  dcw, emb);
    blit(0,       mt,      ml,  sch, dx,         dy+emt,     eml, dch);
    blit(tw-mr,   mt,      mr,  sch, dx+dw-emr,  dy+emt,     emr, dch);

    if (!skipCenter)
        blit(ml, mt, scw, sch, dx+eml, dy+emt, dcw, dch);
}


void TSeBitmapObject::DrawText(SDL_Renderer *r, TTF_Font *font, SDL_Color color,
                               const std::string &text, const SDL_Rect &rect,
                               const std::string &align) {
    if (text.empty() || !font) return;

    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return;

    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, surf);
    int tw = surf->w, th = surf->h;
    SDL_FreeSurface(surf);

    tw = std::min(tw, rect.w);

    SDL_Rect dst;
    dst.w = tw;
    dst.h = th;
    dst.y = rect.y + (rect.h - th) / 2;

    if (align == "taCenter")
        dst.x = rect.x + (rect.w - tw) / 2;
    else if (align == "taRight")
        dst.x = rect.x + rect.w - tw;
    else
        dst.x = rect.x;

    SDL_Rect src = {0, 0, tw, th};
    SDL_RenderCopy(r, tex, &src, &dst);
    SDL_DestroyTexture(tex);
}
