#include "VclRenderer.h"
#include <fstream>
#include <sstream>
#include <algorithm>

VclRenderer::~VclRenderer() {
    for (auto &[k, t] : m_texCache) SDL_DestroyTexture(t);
    for (auto &[k, f] : m_fontCache) TTF_CloseFont(f);
    if (m_labelFont) TTF_CloseFont(m_labelFont);
    if (m_buttonFont) TTF_CloseFont(m_buttonFont);
    if (m_titleFont) TTF_CloseFont(m_titleFont);
    if (m_atlas) SDL_DestroyTexture(m_atlas);
}

bool VclRenderer::load(SDL_Renderer *renderer, const std::string &themeJsonPath,
                       const std::string &assetsDir, const std::string &fontsDir) {
    m_renderer = renderer;
    m_assetsDir = assetsDir;
    m_fontsDir = fontsDir;

    // Load theme.json
    std::ifstream f(themeJsonPath);
    if (!f.is_open()) return false;
    m_theme = json::parse(f, nullptr, false);
    if (m_theme.is_discarded()) return false;

    m_objects = m_theme["objects"];
    m_colors = m_theme["colors"];
    m_sysColors = m_theme["sys_colors"];
    m_fonts = m_theme["fonts"];

    // Load atlas
    m_atlas = IMG_LoadTexture(renderer, (assetsDir + "/style.png").c_str());

    // Load fonts
    // setup.iss hardcodes: Arial Bold 9pt for labels
    m_labelFont = TTF_OpenFont((fontsDir + "/Arial_Bold.ttf").c_str(), 12); // 9pt ≈ 12px
    // Theme button font
    m_buttonFont = TTF_OpenFont((fontsDir + "/tahoma.ttf").c_str(), 11); // 8pt ≈ 11px
    // Theme title font (bold)
    m_titleFont = TTF_OpenFont((fontsDir + "/tahomabd.ttf").c_str(), 11);

    return m_atlas != nullptr;
}

// =============================================================================
// Object tree queries - same as Delphi's GetObjectByName
// =============================================================================

json VclRenderer::findObject(const std::string &path) const {
    std::vector<std::string> parts;
    std::istringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '/')) parts.push_back(part);
    return findIn(m_objects, parts, 0);
}

json VclRenderer::findIn(const json &arr, const std::vector<std::string> &path, int depth) const {
    if (depth >= (int)path.size() || !arr.is_array()) return {};
    for (const auto &o : arr) {
        if (o.value("_name", "") == path[depth]) {
            if (depth == (int)path.size() - 1) return o;
            if (o.contains("_children"))
                return findIn(o["_children"], path, depth + 1);
            return {};
        }
    }
    return {};
}

json VclRenderer::findChild(const json &parent, const std::string &name) const {
    if (!parent.contains("_children")) return {};
    for (const auto &c : parent["_children"])
        if (c.value("_name", "") == name) return c;
    return {};
}

int VclRenderer::getInt(const json &obj, const std::string &key, int def) const {
    if (obj.is_null() || !obj.is_object() || !obj.contains(key)) return def;
    return obj[key].is_number() ? obj[key].get<int>() : def;
}

std::string VclRenderer::getString(const json &obj, const std::string &key, const std::string &def) const {
    if (obj.is_null() || !obj.is_object() || !obj.contains(key)) return def;
    return obj[key].is_string() ? obj[key].get<std::string>() : def;
}

SDL_Rect VclRenderer::getBitmapRect(const json &obj, const std::string &prefix) const {
    if (obj.is_null() || !obj.is_object()) return {0,0,0,0};
    std::string lk = prefix + ".Left", tk = prefix + ".Top",
                rk = prefix + ".Right", bk = prefix + ".Bottom";
    if (!obj.contains(lk)) return {0,0,0,0};
    int l = getInt(obj, lk, 0), t = getInt(obj, tk, 0),
        r = getInt(obj, rk, 0), b = getInt(obj, bk, 0);
    if (r <= l || b <= t) return {0,0,0,0};
    return {l, t, r - l, b - t};
}

void VclRenderer::getMargins(const json &obj, int &ml, int &mt, int &mr, int &mb) const {
    ml = getInt(obj, "MarginLeft", 0);
    mt = getInt(obj, "MarginTop", 0);
    mr = getInt(obj, "MarginRight", 0);
    mb = getInt(obj, "MarginBottom", 0);
}

// =============================================================================
// Color parsing
// =============================================================================

SDL_Color VclRenderer::parseColor(const std::string &hex) const {
    if (hex.size() < 7 || hex[0] != '#') return {0,0,0,255};
    int r = std::stoi(hex.substr(1,2), nullptr, 16);
    int g = std::stoi(hex.substr(3,2), nullptr, 16);
    int b = std::stoi(hex.substr(5,2), nullptr, 16);
    return {(Uint8)r, (Uint8)g, (Uint8)b, 255};
}

SDL_Color VclRenderer::color(const std::string &key) const {
    if (!m_colors.contains(key)) return {0,0,0,255};
    return parseColor(m_colors[key].get<std::string>());
}

SDL_Color VclRenderer::sysColor(const std::string &key) const {
    if (!m_sysColors.contains(key)) return {0,0,0,255};
    return parseColor(m_sysColors[key].get<std::string>());
}

SDL_Color VclRenderer::fontColor(const std::string &key) const {
    if (!m_fonts.contains(key)) return {255,255,255,255};
    auto &f = m_fonts[key];
    if (!f.contains("color")) return {255,255,255,255};
    return parseColor(f["color"].get<std::string>());
}

TTF_Font *VclRenderer::getFont(const std::string &key) {
    // For now return the button font as default
    return m_buttonFont;
}

TTF_Font *VclRenderer::getLabelFont() { return m_labelFont; }
TTF_Font *VclRenderer::getButtonFont() { return m_buttonFont; }
TTF_Font *VclRenderer::getTitleFont() { return m_titleFont; }

// =============================================================================
// Texture loading
// =============================================================================

SDL_Texture *VclRenderer::loadTexture(const std::string &filename) {
    auto it = m_texCache.find(filename);
    if (it != m_texCache.end()) return it->second;
    SDL_Texture *t = IMG_LoadTexture(m_renderer, (m_assetsDir + "/" + filename).c_str());
    if (t) m_texCache[filename] = t;
    return t;
}

// =============================================================================
// Port of TSeBitmapObject.DrawRect (StyleAPI.inc:5334-5375)
//
// Delphi: procedure TSeBitmapObject.DrawRect(Canvas, MarginRect, MarginDstRect, ATileStyle, AMasked)
// Our equivalent: drawRect(src, dst, tileStyle)
// Uses the atlas texture.
// =============================================================================

void VclRenderer::drawRect(const SDL_Rect &src, const SDL_Rect &dst,
                           const std::string &tileStyle) {
    if (src.w <= 0 || src.h <= 0 || dst.w <= 0 || dst.h <= 0) return;

    if (tileStyle == "tsTile" || tileStyle == "tsHorzCenterTile" || tileStyle == "tsVertCenterTile") {
        // Port of: FBitmap.Image.Tile(Canvas, R, MarginRect)
        for (int ty = 0; ty < dst.h; ty += src.h) {
            for (int tx = 0; tx < dst.w; tx += src.w) {
                int cw = std::min(src.w, dst.w - tx);
                int ch = std::min(src.h, dst.h - ty);
                SDL_Rect s = {src.x, src.y, cw, ch};
                SDL_Rect d = {dst.x + tx, dst.y + ty, cw, ch};
                SDL_RenderCopy(m_renderer, m_atlas, &s, &d);
            }
        }
    } else if (tileStyle == "tsCenter") {
        // Port of: FBitmap.Image.Draw(Canvas, centered_X, centered_Y, MarginRect)
        SDL_Rect d = {dst.x + (dst.w - src.w) / 2, dst.y + (dst.h - src.h) / 2, src.w, src.h};
        SDL_Rect s = src;
        SDL_RenderCopy(m_renderer, m_atlas, &s, &d);
    } else {
        // tsStretch (default): FBitmap.Image.Draw(Canvas, R, MarginRect)
        SDL_Rect s = src;
        SDL_Rect d = dst;
        SDL_RenderCopy(m_renderer, m_atlas, &s, &d);
    }
}

// =============================================================================
// Port of TSeBitmapObject.DrawNormal (StyleAPI.inc:5377-5460)
// =============================================================================

void VclRenderer::drawNormal(SDL_Texture *tex, const SDL_Rect &dst,
                             const std::string &tileStyle) {
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
                SDL_RenderCopy(m_renderer, tex, &s, &d);
            }
        }
    } else if (tileStyle == "tsCenter") {
        SDL_Rect d = {dst.x + (dst.w - tw)/2, dst.y + (dst.h - th)/2, tw, th};
        SDL_RenderCopy(m_renderer, tex, nullptr, &d);
    } else {
        // tsStretch
        SDL_Rect d = dst;
        SDL_RenderCopy(m_renderer, tex, nullptr, &d);
    }
}

// =============================================================================
// Port of TSeBitmapObject.Draw (StyleAPI.inc:5486-5604)
// THE 9-PATCH RENDERER - the core of VCL style rendering.
//
// srcRect: region in the atlas texture
// ml,mt,mr,mb: 9-patch margins (from MarginLeft/Top/Right/Bottom)
// dst: destination rect on screen
//
// Divides srcRect into 9 areas using margins:
//   [TL corner] [Top border   ] [TR corner]
//   [L  border] [Center       ] [R  border]
//   [BL corner] [Bottom border] [BR corner]
//
// Corners: drawn at exact size (never stretched)
// Borders: stretched (or tiled per borderTileStyle)
// Center: stretched (or tiled per tileStyle)
// =============================================================================

void VclRenderer::draw9Patch(const SDL_Rect &srcRect, int ml, int mt, int mr, int mb,
                             const SDL_Rect &dst, const std::string &borderTileStyle) {
    if (dst.w <= 0 || dst.h <= 0) return;

    // No margins → simple draw (port of the early-exit in TSeBitmapObject.Draw)
    if (ml == 0 && mt == 0 && mr == 0 && mb == 0) {
        SDL_Rect s = srcRect, d = dst;
        SDL_RenderCopy(m_renderer, m_atlas, &s, &d);
        return;
    }

    int sw = srcRect.w, sh = srcRect.h;
    int sx = srcRect.x, sy = srcRect.y;
    int dw = dst.w, dh = dst.h;
    int dx = dst.x, dy = dst.y;

    // Clamp margins
    int eml = std::min(ml, dw / 2);
    int emr = std::min(mr, dw - eml);
    int emt = std::min(mt, dh / 2);
    int emb = std::min(mb, dh - emt);

    int scw = sw - ml - mr;   // source center width
    int sch = sh - mt - mb;   // source center height
    int dcw = dw - eml - emr; // dest center width
    int dch = dh - emt - emb; // dest center height

    auto blit = [&](int sx2, int sy2, int sw2, int sh2, int dx2, int dy2, int dw2, int dh2) {
        if (sw2 <= 0 || sh2 <= 0 || dw2 <= 0 || dh2 <= 0) return;
        SDL_Rect s = {sx2, sy2, sw2, sh2};
        SDL_Rect d = {dx2, dy2, dw2, dh2};
        SDL_RenderCopy(m_renderer, m_atlas, &s, &d);
    };

    // 4 corners (exact pixel copy - port of Draw(Canvas, X, Y, SrcRect))
    blit(sx, sy, ml, mt, dx, dy, eml, emt);                         // TL
    blit(sx+sw-mr, sy, mr, mt, dx+dw-emr, dy, emr, emt);            // TR
    blit(sx, sy+sh-mb, ml, mb, dx, dy+dh-emb, eml, emb);            // BL
    blit(sx+sw-mr, sy+sh-mb, mr, mb, dx+dw-emr, dy+dh-emb, emr, emb); // BR

    // 4 borders (stretched - port of DrawRect calls)
    blit(sx+ml, sy, scw, mt, dx+eml, dy, dcw, emt);                 // Top
    blit(sx+ml, sy+sh-mb, scw, mb, dx+eml, dy+dh-emb, dcw, emb);   // Bottom
    blit(sx, sy+mt, ml, sch, dx, dy+emt, eml, dch);                 // Left
    blit(sx+sw-mr, sy+mt, mr, sch, dx+dw-emr, dy+emt, emr, dch);   // Right

    // Center (stretched)
    blit(sx+ml, sy+mt, scw, sch, dx+eml, dy+emt, dcw, dch);
}

// =============================================================================
// draw9PatchTex - same as draw9Patch but with a pre-cropped texture
// =============================================================================

void VclRenderer::draw9PatchTex(SDL_Texture *tex, int ml, int mt, int mr, int mb,
                                const SDL_Rect &dst, bool skipCenter) {
    if (!tex || dst.w <= 0 || dst.h <= 0) return;
    int tw, th;
    SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
    SDL_Rect src = {0, 0, tw, th};

    if (ml == 0 && mt == 0 && mr == 0 && mb == 0) {
        SDL_Rect d = dst;
        SDL_RenderCopy(m_renderer, tex, &src, &d);
        return;
    }

    int dw = dst.w, dh = dst.h, dx = dst.x, dy = dst.y;
    int eml = std::min(ml, dw/2), emr = std::min(mr, dw-eml);
    int emt = std::min(mt, dh/2), emb = std::min(mb, dh-emt);
    int scw = tw-ml-mr, sch = th-mt-mb, dcw = dw-eml-emr, dch = dh-emt-emb;

    auto blit = [&](int sx,int sy,int sw,int sh, int dx2,int dy2,int dw2,int dh2) {
        if (sw<=0||sh<=0||dw2<=0||dh2<=0) return;
        SDL_Rect s={sx,sy,sw,sh}, d={dx2,dy2,dw2,dh2};
        SDL_RenderCopy(m_renderer, tex, &s, &d);
    };

    blit(0,0,ml,mt, dx,dy,eml,emt);
    blit(tw-mr,0,mr,mt, dx+dw-emr,dy,emr,emt);
    blit(0,th-mb,ml,mb, dx,dy+dh-emb,eml,emb);
    blit(tw-mr,th-mb,mr,mb, dx+dw-emr,dy+dh-emb,emr,emb);
    blit(ml,0,scw,mt, dx+eml,dy,dcw,emt);
    blit(ml,th-mb,scw,mb, dx+eml,dy+dh-emb,dcw,emb);
    blit(0,mt,ml,sch, dx,dy+emt,eml,dch);
    blit(tw-mr,mt,mr,sch, dx+dw-emr,dy+emt,emr,dch);
    if (!skipCenter)
        blit(ml,mt,scw,sch, dx+eml,dy+emt,dcw,dch);
}

// =============================================================================
// Text drawing
// =============================================================================

void VclRenderer::drawText(const std::string &text, TTF_Font *font, SDL_Color color,
                           const SDL_Rect &rect, const std::string &align) {
    if (text.empty() || !font) return;

    SDL_Surface *surf = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surf) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(m_renderer, surf);

    int tw = surf->w, th = surf->h;
    SDL_FreeSurface(surf);

    // Clip to rect
    tw = std::min(tw, rect.w);

    SDL_Rect dst;
    dst.w = tw;
    dst.h = th;

    // Vertical: always center
    dst.y = rect.y + (rect.h - th) / 2;

    // Horizontal alignment (port of DrawObjecTSext text alignment)
    if (align == "taCenter") {
        dst.x = rect.x + (rect.w - tw) / 2;
    } else if (align == "taRight") {
        dst.x = rect.x + rect.w - tw;
    } else {
        dst.x = rect.x; // taLeft
    }

    SDL_Rect src = {0, 0, tw, th};
    SDL_RenderCopy(m_renderer, tex, &src, &dst);
    SDL_DestroyTexture(tex);
}
