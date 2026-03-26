#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>
#include <map>
#include <vector>

// Forward declaration for JSON parsing (we use a simple built-in parser)
#include "json.hpp"
using json = nlohmann::json;

/**
 * Direct 1:1 port of Delphi's VCL style rendering to SDL2.
 *
 * Delphi equivalents:
 *   TseBitmap.Draw(Canvas, DstRect, SrcRect) → SDL_RenderCopy(renderer, texture, &src, &dst)
 *   TSeBitmapObject.Draw         → VclRenderer::drawObject()
 *   TSeBitmapObject.DrawNormal   → VclRenderer::drawNormal()
 *   TSeBitmapObject.DrawRect     → VclRenderer::drawRect()
 *   TSeStyle.ButtonDraw          → VclRenderer::drawButton()
 *   TSeStyle.CheckDraw           → VclRenderer::drawCheckBox()
 *   TSeStyle.EditDraw            → VclRenderer::drawEdit()
 *   TSeStyle.ProgressDraw        → VclRenderer::drawProgress()
 */
class VclRenderer {
public:
    ~VclRenderer();

    bool load(SDL_Renderer *renderer, const std::string &themeJsonPath,
              const std::string &assetsDir, const std::string &fontsDir);

    // --- Style object queries (from theme.json) ---
    json findObject(const std::string &path) const;
    json findChild(const json &parent, const std::string &name) const;

    int getInt(const json &obj, const std::string &key, int def = 0) const;
    std::string getString(const json &obj, const std::string &key, const std::string &def = "") const;
    SDL_Rect getBitmapRect(const json &obj, const std::string &prefix = "Bitmap") const;
    void getMargins(const json &obj, int &ml, int &mt, int &mr, int &mb) const;

    // --- Color/font from theme tables ---
    SDL_Color color(const std::string &key) const;
    SDL_Color sysColor(const std::string &key) const;
    SDL_Color fontColor(const std::string &key) const;
    TTF_Font *getFont(const std::string &key);  // returns cached font
    TTF_Font *getLabelFont();  // Arial Bold 9pt (setup.iss hardcoded)
    TTF_Font *getButtonFont(); // from theme ktfButtonTextNormal
    TTF_Font *getTitleFont();  // from theme ktfCaptionTextNormal

    // --- Atlas texture ---
    SDL_Texture *atlas() const { return m_atlas; }

    // --- Drawing: 1:1 ports of Delphi functions ---

    /**
     * Port of TSeBitmapObject.DrawRect (StyleAPI.inc:5334)
     * Draws a single source rect to a dest rect with tile mode handling.
     */
    void drawRect(const SDL_Rect &src, const SDL_Rect &dst,
                  const std::string &tileStyle = "tsStretch");

    /**
     * Port of TSeBitmapObject.DrawNormal (StyleAPI.inc:5377)
     * Draw without margins - stretch/tile/center based on tileStyle.
     */
    void drawNormal(SDL_Texture *tex, const SDL_Rect &dst,
                    const std::string &tileStyle = "tsStretch");

    /**
     * Port of TSeBitmapObject.Draw (StyleAPI.inc:5486)
     * 9-patch drawing with margins. The core rendering function.
     * Uses the atlas texture with source rect.
     */
    void draw9Patch(const SDL_Rect &srcRect, int ml, int mt, int mr, int mb,
                    const SDL_Rect &dst, const std::string &borderTileStyle = "tsStretch");

    /**
     * Draw a pre-cropped texture with 9-patch margins.
     */
    void draw9PatchTex(SDL_Texture *tex, int ml, int mt, int mr, int mb,
                       const SDL_Rect &dst, bool skipCenter = false);

    // --- Text drawing ---
    void drawText(const std::string &text, TTF_Font *font, SDL_Color color,
                  const SDL_Rect &rect, const std::string &align = "taLeft");

    // --- Convenience: draw named asset PNG ---
    SDL_Texture *loadTexture(const std::string &filename);

    SDL_Renderer *renderer() const { return m_renderer; }

private:
    SDL_Renderer *m_renderer = nullptr;
    SDL_Texture *m_atlas = nullptr;
    json m_theme;
    json m_objects;
    json m_colors;
    json m_sysColors;
    json m_fonts;
    std::string m_assetsDir;
    std::string m_fontsDir;

    // Font cache
    std::map<std::string, TTF_Font*> m_fontCache;
    TTF_Font *m_labelFont = nullptr;   // Arial Bold 9pt
    TTF_Font *m_buttonFont = nullptr;
    TTF_Font *m_titleFont = nullptr;

    // Texture cache
    std::map<std::string, SDL_Texture*> m_texCache;

    json findIn(const json &arr, const std::vector<std::string> &path, int depth) const;
    SDL_Color parseColor(const std::string &hex) const;
};
