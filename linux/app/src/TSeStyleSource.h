#pragma once

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string>
#include <map>
#include <vector>

#include "../include/json.hpp"
using json = nlohmann::json;


class TSeStyleSource {
public:
    ~TSeStyleSource();

    bool LoadFromFile(SDL_Renderer *renderer, const std::string &themeJson,
                      const std::string &assetsDir, const std::string &fontsDir);

    // TSeStyleSource.GetObjectByName
    json GetObjectByName(const std::string &path) const;
    json FindChild(const json &parent, const std::string &name) const;

    int    GetInt(const json &obj, const std::string &key, int def = 0) const;
    std::string GetString(const json &obj, const std::string &key, const std::string &def = "") const;

    SDL_Rect GetBitmapRect(const json &obj, const std::string &prefix = "Bitmap") const;
    void GetMargins(const json &obj, int &ml, int &mt, int &mr, int &mb) const;

    // TSeStyleColors / TSeStyleSysColors / TSeStyleFonts
    SDL_Color  GetColor(const std::string &key) const;
    SDL_Color  GetSysColor(const std::string &key) const;
    SDL_Color  GetFontColor(const std::string &key) const;

    TTF_Font *LabelFont();
    TTF_Font *ButtonFont();
    TTF_Font *CaptionFont();

    SDL_Texture *LoadTexture(const std::string &filename);

    SDL_Renderer *Renderer() const { return FRenderer; }

private:
    SDL_Renderer *FRenderer = nullptr;
    json FObjects;
    json FColors;
    json FSysColors;
    json FFonts;
    std::string FAssetsDir;

    TTF_Font *FLabelFont   = nullptr;
    TTF_Font *FButtonFont  = nullptr;
    TTF_Font *FCaptionFont = nullptr;

    std::map<std::string, SDL_Texture *> FTexCache;

    json FindIn(const json &arr, const std::vector<std::string> &path, int depth) const;
    SDL_Color ParseColor(const std::string &hex) const;
};
