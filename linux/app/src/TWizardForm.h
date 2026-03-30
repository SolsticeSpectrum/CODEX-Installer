#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <vector>

#include "TSeStyleSource.h"
#include "ISExtractor.h"
#include "config.h"


class TWizardForm {
public:
    bool InitializeSetup(const std::string &assetsDir, const std::string &themeDir,
                         const std::string &layoutPath, const std::string &fontsDir,
                         const std::string &logoNum, const std::string &iconNum,
                         const std::string &musicNum);

    void SetSourceDir(const std::string &dir) { FSourceDir = dir; }
    void Run();
    void DeinitializeSetup();

private:
    enum ISStep { wpSelectDir, wpInstalling, wpFinished };

    // TFormStyleHook.PaintNC
    void PaintNC();

    // rendering
    void Paint();
    void EditDraw();
    void ButtonDraw();
    void CheckDraw();
    void ProgressDraw();
    void LabelDraw();
    void AudioDraw();
    void ResultDraw();

    // events
    void HandleEvent(const SDL_Event &e);
    void WMMouseDown(int x, int y);
    void WMMouseUp(int x, int y);
    void WMMouseMove(int x, int y);
    void WMTextInput(const char *text);
    void WMKeyDown(SDL_Keycode key);

    // setup.iss CurPageChanged
    void CurPageChanged(ISStep step);

    // layout rect from layout.json, offset by border + titlebar
    SDL_Rect LayoutRect(const std::string &name) const;

    TSeStyleSource FStyleSource;
    json           FLayout;
    std::string    FAssetsDir;
    std::string    FSourceDir;  // {src} — dir containing installer + .bin files
    SDL_Window    *FWindow   = nullptr;
    SDL_Renderer  *FRenderer = nullptr;
    bool           FRunning  = true;
    ISStep         FStep     = wpSelectDir;

    int FTitleH   = 0;
    int FBorderL  = 0;
    int FBorderR  = 0;
    int FBorderB  = 0;
    int FWindowH  = 0;

    // text fields
    struct TNewEdit {
        std::string Text;
        std::string Name;
        int  Cursor  = 0;
        bool Focused = false;
        bool Enabled = true;
    };
    TNewEdit DirEdit, GroupEdit;
    int ActiveDrive = 0;
    std::vector<std::string> Drives;

    // checkboxes
    bool chbCreateDesktopIcon  = true;
    bool chbCreateGroup        = true;
    bool chbNoUninstaller      = false;
    bool chbDesktopIconHover   = false;
    bool chbCreateGroupHover   = false;
    bool chbNoUninstallerHover = false;

    // buttons
    struct TButton {
        SDL_Rect Rect;
        std::string Caption;
        bool Hover   = false;
        bool Pressed = false;
        bool Enabled = true;
        bool Visible = true;
    };
    TButton btnLeftButton, btnRightButton, btnPause;
    TButton btnDirBrowse, btnGroupBrowse;

    // window buttons
    SDL_Rect FCloseButtonRect = {};
    SDL_Rect FMinButtonRect   = {};
    bool     FCloseHover = false;
    bool     FMinHover   = false;

    // progress
    ISExtractor FExtractor;
    int  ProgressValue = 0;
    bool ISPaused      = false;
    bool ShowResult    = false;

    // log
    std::vector<std::string> LogLines;
    int  FLogScroll     = 0;
    int  FLogMaxScroll  = 0;
    int  FLogMaxLines   = 0;
    int  FLogPrevMax    = 0;
    bool FLogDragging   = false;
    bool FLogThumbHover = false;
    int  FLogDragY      = 0;
    int  FLogDragStart  = 0;
    SDL_Rect FLogUpR = {}, FLogDnR = {}, FLogThumbR = {};
    int  FLogTrackTop = 0, FLogTrackH = 0;

    // drag
    bool FDragging = false;
    int  FDragX = 0, FDragY = 0;

    // audio
    Mix_Music *FMusic = nullptr;

    // textures
    SDL_Texture *FLogoTex    = nullptr;
    SDL_Texture *FIconTex    = nullptr;

    SDL_Texture *FBtnNormal   = nullptr;
    SDL_Texture *FBtnHot      = nullptr;
    SDL_Texture *FBtnPressed  = nullptr;
    SDL_Texture *FBtnDisabled = nullptr;
    int FBtnML = 0, FBtnMT = 0, FBtnMR = 0, FBtnMB = 0;

    SDL_Texture *FEditTex = nullptr;
    int FEdML = 0, FEdMT = 0, FEdMR = 0, FEdMB = 0;
    int FEdFrameML = 0, FEdFrameMT = 0, FEdFrameMR = 0, FEdFrameMB = 0;

    SDL_Texture *FComboTex      = nullptr;
    int FCbML = 0, FCbMT = 0, FCbMR = 0, FCbMB = 0;
    SDL_Texture *FComboBtnN     = nullptr;
    SDL_Texture *FComboBtnH     = nullptr;
    SDL_Texture *FComboBtnP     = nullptr;
    SDL_Texture *FComboBtnD     = nullptr;
    int FCbBtnML = 0, FCbBtnMT = 0, FCbBtnMR = 0, FCbBtnMB = 0;
    SDL_Texture *FComboArrowTex = nullptr;

    SDL_Texture *FChkUncheckedN = nullptr, *FChkUncheckedH = nullptr, *FChkUncheckedD = nullptr;
    SDL_Texture *FChkCheckedN   = nullptr, *FChkCheckedH   = nullptr, *FChkCheckedD   = nullptr;

    SDL_Texture *FProgFrameTex = nullptr;
    SDL_Texture *FProgBarTex   = nullptr;
    int FPfML = 0, FPfMT = 0, FPfMR = 0, FPfMB = 0;
    int FPbML = 0, FPbMT = 0, FPbMR = 0, FPbMB = 0;

    SDL_Texture *FScrollVfTex = nullptr;
    int FSvfML = 0, FSvfMT = 0, FSvfMR = 0, FSvfMB = 0;

    SDL_Texture *FScrollTopN      = nullptr, *FScrollTopH      = nullptr, *FScrollTopP = nullptr, *FScrollTopD = nullptr;
    SDL_Texture *FScrollBotN      = nullptr, *FScrollBotH      = nullptr, *FScrollBotP = nullptr, *FScrollBotD = nullptr;
    SDL_Texture *FScrollTopArrowN = nullptr, *FScrollTopArrowD = nullptr;
    SDL_Texture *FScrollBotArrowN = nullptr, *FScrollBotArrowD = nullptr;
    int FSbBtnML = 0, FSbBtnMT = 0, FSbBtnMR = 0, FSbBtnMB = 0;
    bool FScrollTopHover   = false, FScrollBotHover   = false;
    bool FScrollTopPressed = false, FScrollBotPressed = false;

    SDL_Texture *FScrollThumbN = nullptr, *FScrollThumbH = nullptr, *FScrollThumbP = nullptr;
    int FStML = 0, FStMT = 0, FStMR = 0, FStMB = 0;

    SDL_Texture *FWndCloseN = nullptr, *FWndCloseH = nullptr;
    SDL_Texture *FWndMinN   = nullptr, *FWndMinH   = nullptr;
    int FWndBtnW = 0, FWndBtnH = 0;

    SDL_Texture *FTitlebarTex  = nullptr;
    int FTitleML = 0, FTitleMT = 0, FTitleMR = 0, FTitleMB = 0;

    SDL_Texture *FBorderLTex = nullptr;
    SDL_Texture *FBorderRTex = nullptr;
    SDL_Texture *FBorderBTex = nullptr;
    int FBlML = 0, FBlMT = 0, FBlMR = 0, FBlMB = 0;
    int FBrML = 0, FBrMT = 0, FBrMR = 0, FBrMB = 0;
    int FBbML = 0, FBbMT = 0, FBbMR = 0, FBbMB = 0;

    SDL_Texture *FClientBgTex = nullptr;
    std::string  FClientTile;

    // title bar
    int         FTitleTextML = 0, FTitleTextMR = 0;
    std::string FTitleTextAlign;
    SDL_Rect    FTitleTextRect = {};
    SDL_Rect    FIconRect      = {};

    // music controls
    SDL_Texture *FPlayN      = nullptr, *FPlayH     = nullptr, *FPlayP     = nullptr;
    SDL_Texture *FPauseN     = nullptr, *FPauseH    = nullptr, *FPauseP    = nullptr;
    SDL_Texture *FTrackBgTex = nullptr;
    SDL_Texture *FTrackBtnN  = nullptr, *FTrackBtnH = nullptr, *FTrackBtnP = nullptr;
    float FVolume         = 0.58f;
    bool  FDraggingVolume = false;
    bool  FPlayHover   = false, FPauseHover   = false, FTrackBtnHover = false;
    bool  FPlayPressed = false, FPausePressed = false;

    // combobox state
    bool FComboHover = false, FComboPressed = false;

    // colors
    SDL_Color FClrBorder    = {};
    SDL_Color FClrEdit      = {};
    SDL_Color FClrLabel     = {};
    SDL_Color FClrEditText  = {};
    SDL_Color FClrTitleText = {};
    SDL_Color FClrBtnTextN  = {}, FClrBtnTextH = {}, FClrBtnTextP = {}, FClrBtnTextD = {};
};
