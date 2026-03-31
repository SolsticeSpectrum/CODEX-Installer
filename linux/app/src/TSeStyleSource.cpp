#include "TSeStyleSource.h"
#include <fstream>
#include <sstream>
#include <algorithm>


TSeStyleSource::~TSeStyleSource() {
    for (auto &[k, t] : FTexCache) SDL_DestroyTexture(t);
    if (FLabelFont)   TTF_CloseFont(FLabelFont);
    if (FButtonFont)  TTF_CloseFont(FButtonFont);
    if (FCaptionFont) TTF_CloseFont(FCaptionFont);
}


bool TSeStyleSource::LoadFromFile(SDL_Renderer *renderer, const std::string &themeJson,
                                  const std::string &assetsDir, const std::string &fontsDir) {

    FRenderer  = renderer;
    FAssetsDir = assetsDir;

    std::ifstream f(themeJson);
    if (!f.is_open()) return false;

    auto theme = json::parse(f, nullptr, false);
    if (theme.is_discarded()) return false;

    FObjects   = theme["objects"];
    FColors    = theme["colors"];
    FSysColors = theme["sys_colors"];
    FFonts     = theme["fonts"];

    // 9pt ~ 12px, 8pt ~ 11px
    FLabelFont   = TTF_OpenFont((fontsDir + "/ArialBd.ttf").c_str(), 12);
    FButtonFont  = TTF_OpenFont((fontsDir + "/tahoma.ttf").c_str(), 11);
    FCaptionFont = TTF_OpenFont((fontsDir + "/tahomabd.ttf").c_str(), 11);

    return true;
}


json TSeStyleSource::GetObjectByName(const std::string &path) const {
    std::vector<std::string> parts;
    std::istringstream ss(path);
    std::string part;
    while (std::getline(ss, part, '/')) parts.push_back(part);

    return FindIn(FObjects, parts, 0);
}


json TSeStyleSource::FindIn(const json &arr, const std::vector<std::string> &path, int depth) const {
    if (depth >= (int)path.size() || !arr.is_array()) return {};

    for (const auto &o : arr) {
        if (o.value("_name", "") == path[depth]) {
            if (depth == (int)path.size() - 1) return o;
            if (o.contains("_children"))
                return FindIn(o["_children"], path, depth + 1);
            return {};
        }
    }

    return {};
}


json TSeStyleSource::FindChild(const json &parent, const std::string &name) const {
    if (!parent.contains("_children")) return {};

    for (const auto &c : parent["_children"])
        if (c.value("_name", "") == name) return c;

    return {};
}


int TSeStyleSource::GetInt(const json &obj, const std::string &key, int def) const {
    if (obj.is_null() || !obj.is_object() || !obj.contains(key)) return def;
    return obj[key].is_number() ? obj[key].get<int>() : def;
}


std::string TSeStyleSource::GetString(const json &obj, const std::string &key, const std::string &def) const {
    if (obj.is_null() || !obj.is_object() || !obj.contains(key)) return def;
    return obj[key].is_string() ? obj[key].get<std::string>() : def;
}


SDL_Rect TSeStyleSource::GetBitmapRect(const json &obj, const std::string &prefix) const {
    if (obj.is_null() || !obj.is_object()) return {0, 0, 0, 0};

    std::string lk = prefix + ".Left",  tk = prefix + ".Top",
                rk = prefix + ".Right", bk = prefix + ".Bottom";

    if (!obj.contains(lk)) return {0, 0, 0, 0};

    int l = GetInt(obj, lk), t = GetInt(obj, tk),
        r = GetInt(obj, rk), b = GetInt(obj, bk);

    if (r <= l || b <= t) return {0, 0, 0, 0};
    return {l, t, r - l, b - t};
}


void TSeStyleSource::GetMargins(const json &obj, int &ml, int &mt, int &mr, int &mb) const {
    ml = GetInt(obj, "MarginLeft");
    mt = GetInt(obj, "MarginTop");
    mr = GetInt(obj, "MarginRight");
    mb = GetInt(obj, "MarginBottom");
}


SDL_Color TSeStyleSource::ParseColor(const std::string &hex) const {
    if (hex.size() < 7 || hex[0] != '#') return {0, 0, 0, 255};

    int r = std::stoi(hex.substr(1, 2), nullptr, 16);
    int g = std::stoi(hex.substr(3, 2), nullptr, 16);
    int b = std::stoi(hex.substr(5, 2), nullptr, 16);

    return {(Uint8)r, (Uint8)g, (Uint8)b, 255};
}


SDL_Color TSeStyleSource::GetColor(const std::string &key) const {
    if (!FColors.contains(key)) return {0, 0, 0, 255};
    return ParseColor(FColors[key].get<std::string>());
}


SDL_Color TSeStyleSource::GetSysColor(const std::string &key) const {
    if (!FSysColors.contains(key)) return {0, 0, 0, 255};
    return ParseColor(FSysColors[key].get<std::string>());
}


SDL_Color TSeStyleSource::GetFontColor(const std::string &key) const {
    if (!FFonts.contains(key)) return {255, 255, 255, 255};
    auto &f = FFonts[key];

    if (!f.contains("color")) return {255, 255, 255, 255};
    return ParseColor(f["color"].get<std::string>());
}


TTF_Font *TSeStyleSource::LabelFont()   { return FLabelFont; }
TTF_Font *TSeStyleSource::ButtonFont()  { return FButtonFont; }
TTF_Font *TSeStyleSource::CaptionFont() { return FCaptionFont; }


SDL_Texture *TSeStyleSource::LoadTexture(const std::string &filename) {
    auto it  = FTexCache.find(filename);
    if  (it != FTexCache.end()) return it->second;

    auto *t = IMG_LoadTexture(FRenderer, (FAssetsDir + "/" + filename).c_str());
    if (t) FTexCache[filename] = t;

    return t;
}
