#pragma once

#include <string>
#include <SDL.h>


// TODO: Qt6 QFileDialog with VCL theme QPalette + button bitmaps
class TSelectFolderForm {
public:
    static std::string Execute(const std::string &title,
                               const std::string &initialDir,
                               SDL_Color btnFace, SDL_Color btnText,
                               SDL_Color windowBg, SDL_Color windowText);
};
