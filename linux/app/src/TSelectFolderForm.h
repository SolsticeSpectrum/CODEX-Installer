#pragma once

#include <string>
#include <SDL.h>


// TODO: open Qt QFileDialog themed with VCL colors + button bitmaps
// link Qt6 just for the dialog, apply QPalette from theme colors
// and minimal QSS for buttons using the 9-patch button textures
class TSelectFolderForm {
public:
    // returns selected path or empty string if cancelled
    static std::string Execute(const std::string &title,
                               const std::string &initialDir,
                               SDL_Color btnFace, SDL_Color btnText,
                               SDL_Color windowBg, SDL_Color windowText);
};
