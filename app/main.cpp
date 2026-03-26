#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <string>
#include <filesystem>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include "InstallerWindow.h"
#include "miniz.h"
#include "assets_data.h"

static std::string extractAssets() {
    // Create temp directory
    char tmpl[] = "/tmp/codex-installer-XXXXXX";
    char *tmpdir = mkdtemp(tmpl);
    if (!tmpdir) { SDL_Log("Cannot create temp dir"); return ""; }
    std::string dir(tmpdir);

    // Extract zip from embedded data
    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_mem(&zip, embedded_assets_zip, embedded_assets_zip_len, 0)) {
        SDL_Log("Cannot open embedded zip");
        return "";
    }

    int n = mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < n; i++) {
        char fname[256];
        mz_zip_reader_get_filename(&zip, i, fname, sizeof(fname));
        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            mkdir((dir + "/" + fname).c_str(), 0755);
            continue;
        }
        // Ensure parent directory exists
        std::string outPath = dir + "/" + fname;
        auto slashPos = outPath.rfind('/');
        if (slashPos != std::string::npos) {
            std::string parentDir = outPath.substr(0, slashPos);
            mkdir(parentDir.c_str(), 0755);
        }
        mz_zip_reader_extract_to_file(&zip, i, outPath.c_str(), 0);
    }
    mz_zip_reader_end(&zip);

    SDL_Log("Extracted %d files to %s", n, dir.c_str());
    return dir;
}

static void cleanupAssets(const std::string &dir) {
    if (dir.empty() || dir == "/") return;
    std::filesystem::remove_all(dir);
}

int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Mix_Init(MIX_INIT_OGG);

    // Extract embedded assets to temp dir
    std::string assetDir = extractAssets();
    if (assetDir.empty()) return 1;

    // Extracted structure: assets/ subfolder for theme PNGs, rest at root
    InstallerWindow win;
    if (!win.init(assetDir, assetDir, assetDir + "/layout.json", assetDir,
                  "5", "1", "1")) {  // TODO: make these configurable per variant
        SDL_Log("Failed to initialize installer window");
        cleanupAssets(assetDir);
        return 1;
    }

    win.run();
    win.cleanup();

    cleanupAssets(assetDir);

    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
