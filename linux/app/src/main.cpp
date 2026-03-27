#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <string>
#include <filesystem>
#include <cstdio>
#include <sys/stat.h>

#include "TWizardForm.h"
#include "../include/miniz.h"
#include "../data/assets_data.h"

#ifndef LOGO_NUM
#define LOGO_NUM "5"
#endif
#ifndef ICON_NUM
#define ICON_NUM "1"
#endif
#ifndef MUSIC_NUM
#define MUSIC_NUM "1"
#endif


static std::string ExtractAssets() {
    char tmpl[] = "/tmp/codex-installer-XXXXXX";
    char *dir = mkdtemp(tmpl);
    if (!dir) return "";

    mz_zip_archive zip = {};
    if (!mz_zip_reader_init_mem(&zip, embedded_assets_zip, embedded_assets_zip_len, 0))
        return "";

    int n = mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < n; i++) {
        char fname[256];
        mz_zip_reader_get_filename(&zip, i, fname, sizeof(fname));

        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            mkdir((std::string(dir) + "/" + fname).c_str(), 0755);
            continue;
        }

        std::string out = std::string(dir) + "/" + fname;
        auto slash = out.rfind('/');
        if (slash != std::string::npos)
            mkdir(out.substr(0, slash).c_str(), 0755);

        mz_zip_reader_extract_to_file(&zip, i, out.c_str(), 0);
    }

    mz_zip_reader_end(&zip);
    return dir;
}


int main(int, char *[]) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    TTF_Init();
    Mix_Init(MIX_INIT_OGG);

    std::string dir = ExtractAssets();
    if (dir.empty()) return 1;

    TWizardForm form;
    if (!form.InitializeSetup(dir, dir, dir + "/layout.json", dir, LOGO_NUM, ICON_NUM, MUSIC_NUM)) {
        std::filesystem::remove_all(dir);
        return 1;
    }

    form.Run();
    form.DeinitializeSetup();
    std::filesystem::remove_all(dir);

    Mix_Quit();
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
