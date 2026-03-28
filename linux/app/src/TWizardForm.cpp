#include "TWizardForm.h"
#include "TSeBitmapObject.h"
#include "DrawStyleEdge.h"
#include <fstream>
#include <filesystem>
#include <cstring>


static bool HitTest(int x, int y, const SDL_Rect &r) {
    return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h;
}


bool TWizardForm::InitializeSetup(const std::string &assetsDir, const std::string &themeDir,
                                   const std::string &layoutPath, const std::string &fontsDir,
                                   const std::string &logoNum, const std::string &iconNum,
                                   const std::string &musicNum) {
    FAssetsDir = assetsDir;

    // layout
    std::ifstream lf(layoutPath);
    if (lf.is_open()) FLayout = json::parse(lf, nullptr, false);

    // read frame dimensions from theme before creating window
    {
        std::ifstream tf(themeDir + "/theme.json");
        if (!tf.is_open()) return false;

        auto theme = json::parse(tf, nullptr, false);
        if (theme.is_discarded()) return false;

        for (const auto &o : theme["objects"]) {
            if (o.value("_name", "") != "Form") continue;
            if (!o.contains("_children")) break;

            for (const auto &c : o["_children"]) {
                if (c.value("_name", "") != "Image") continue;
                if (!c.contains("_children")) break;

                for (const auto &gc : c["_children"]) {
                    std::string n = gc.value("_name", "");
                    
                    if (n == "Title")        FTitleH  = gc.value("Height", 30);
                    else if (n == "LeftBorder")  FBorderL = gc.value("Width", 8);
                    else if (n == "RightBorder") FBorderR = gc.value("Width", 8);
                    else if (n == "BottomBorder") FBorderB = gc.value("Height", 8);
                }

                break;
            }

            break;
        }
    }

    int winW = FBorderL + ClientWidth + FBorderR;
    FWindowH = FTitleH + ClientHeight + FBorderB;

    FWindow = SDL_CreateWindow("Example Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW, FWindowH, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!FWindow) return false;

    FRenderer = SDL_CreateRenderer(FWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!FRenderer) return false;

    if (!FStyleSource.LoadFromFile(FRenderer, themeDir + "/theme.json", themeDir + "/assets", fontsDir))
        return false;

    SDL_StartTextInput();
    auto load = [&](const std::string &f) { return FStyleSource.LoadTexture(f); };

    // titlebar
    auto titleObj = FStyleSource.GetObjectByName("Form/Image/Title");
    FStyleSource.GetMargins(titleObj, FTitleML, FTitleMT, FTitleMR, FTitleMB);
    FTitlebarTex  = load("titlebar_active.png");

    // borders
    auto lbObj  = FStyleSource.GetObjectByName("Form/Image/LeftBorder");
    FStyleSource.GetMargins(lbObj, FBlML, FBlMT, FBlMR, FBlMB);
    FBorderLTex = load("border_leftborder_active.png");

    auto rbObj  = FStyleSource.GetObjectByName("Form/Image/RightBorder");
    FStyleSource.GetMargins(rbObj, FBrML, FBrMT, FBrMR, FBrMB);
    FBorderRTex = load("border_rightborder_active.png");

    auto bbObj  = FStyleSource.GetObjectByName("Form/Image/BottomBorder");
    FStyleSource.GetMargins(bbObj, FBbML, FBbMT, FBbMR, FBbMB);
    FBorderBTex = load("border_bottomborder_active.png");

    // correct border sizes from actual bitmaps
    if (FBorderLTex) { int tw; SDL_QueryTexture(FBorderLTex, nullptr, nullptr, &tw, nullptr); FBorderL = tw; }
    if (FBorderRTex) { int tw; SDL_QueryTexture(FBorderRTex, nullptr, nullptr, &tw, nullptr); FBorderR = tw; }
    if (FBorderBTex) { int th; SDL_QueryTexture(FBorderBTex, nullptr, nullptr, nullptr, &th); FBorderB = th; }
    winW = FBorderL + ClientWidth + FBorderR;
    FWindowH = FTitleH + ClientHeight + FBorderB;
    SDL_SetWindowSize(FWindow, winW, FWindowH);

    // client bg
    auto clientObj = FStyleSource.GetObjectByName("Form/Image/Client");
    FClientBgTex   = load("form_client.png");
    FClientTile    = FStyleSource.GetString(clientObj, "TileStyle", "tsTile");

    // button face
    auto btnFace = FStyleSource.GetObjectByName("Button/Face");
    FStyleSource.GetMargins(btnFace, FBtnML, FBtnMT, FBtnMR, FBtnMB);
    FBtnNormal   = load("button_normal.png");
    FBtnHot      = load("button_hot.png");
    FBtnPressed  = load("button_pressed.png");
    FBtnDisabled = load("button_disabled.png");

    // edit frame
    auto editFrame  = FStyleSource.GetObjectByName("Edit/Frame");
    FStyleSource.GetMargins(editFrame, FEdFrameML, FEdFrameMT, FEdFrameMR, FEdFrameMB);
    auto editBitmap = FStyleSource.GetObjectByName("Edit/Frame/bitmap");
    FStyleSource.GetMargins(editBitmap, FEdML, FEdMT, FEdMR, FEdMB);
    FEditTex        = load("edit_frame.png");

    // combobox
    auto cbFrame   = FStyleSource.GetObjectByName("ComboBox/Frame/bitmap");
    FStyleSource.GetMargins(cbFrame, FCbML, FCbMT, FCbMR, FCbMB);
    FComboTex      = load("combobox_frame.png");
    FComboArrowTex = load("combobox_arrow_normal.png");

    auto cbBtn = FStyleSource.GetObjectByName("ComboBox/Button");
    FStyleSource.GetMargins(cbBtn, FCbBtnML, FCbBtnMT, FCbBtnMR, FCbBtnMB);
    FComboBtnN = load("combobox_button_normal.png");
    FComboBtnH = load("combobox_button_hot.png");

    // progressbar
    auto progFrame = FStyleSource.GetObjectByName("ProgressBar/Frame");
    FStyleSource.GetMargins(progFrame, FPfML, FPfMT, FPfMR, FPfMB);
    FProgFrameTex  = load("progressbar_frame.png");
    auto progBar   = FStyleSource.GetObjectByName("ProgressBar/BarHorz");
    FStyleSource.GetMargins(progBar, FPbML, FPbMT, FPbMR, FPbMB);
    FProgBarTex    = load("progressbar_bar.png");

    // checkboxes
    FChkUncheckedN = load("checkbox_unchecked_normal.png");
    FChkUncheckedH = load("checkbox_unchecked_hot.png");
    FChkUncheckedD = load("checkbox_unchecked_disabled.png");
    FChkCheckedN   = load("checkbox_checked_normal.png");
    FChkCheckedH   = load("checkbox_checked_hot.png");
    FChkCheckedD   = load("checkbox_checked_disabled.png");

    // window buttons
    auto btnClose = FStyleSource.GetObjectByName("Form/Image/Title/Caption/sysButtons/btnClose");
    FWndBtnW   = FStyleSource.GetInt(btnClose, "Width", 20);
    FWndBtnH   = FStyleSource.GetInt(btnClose, "Height", 28);
    FWndCloseN = load("wnd_btnClose_normal.png");
    FWndCloseH = load("wnd_btnClose_hot.png");
    FWndMinN   = load("wnd_btnMin_normal.png");
    FWndMinH   = load("wnd_btnMin_hot.png");

    // title bar button layout
    auto captionObj = FStyleSource.GetObjectByName("Form/Image/Title/Caption");
    int capW = FStyleSource.GetInt(captionObj, "Width", 300);
    int capX = (winW - FBorderR + 1) - capW;
    int capY = FStyleSource.GetInt(captionObj, "Top", 2);

    auto capLeftObj = FStyleSource.FindChild(captionObj, "CaptionLeft");
    int capLeftW    = FStyleSource.GetInt(capLeftObj, "Width", 9);
    int sysX = capX + capLeftW;
    int sysW = capW - capLeftW;

    int rightEdge = sysX + sysW;

    int closeX = rightEdge - FWndBtnW;
    int minX   = closeX - FWndBtnW;
    FCloseButtonRect = {closeX, capY, FWndBtnW, FWndBtnH};
    FMinButtonRect   = {minX,   capY, FWndBtnW, FWndBtnH};

    // all 5 slots for text centering
    int helpX = rightEdge - 5 * FWndBtnW;
    auto capTitle = FStyleSource.GetObjectByName("Form/Image/Title/Caption/sysButtons/CaptionTitle");
    FTitleTextML    = FStyleSource.GetInt(capTitle, "TextMarginLeft", 0);
    FTitleTextMR    = FStyleSource.GetInt(capTitle, "TextMarginRight", 90);
    FTitleTextAlign = FStyleSource.GetString(capTitle, "TextAlign", "taCenter");

    auto sysMenuObj = FStyleSource.GetObjectByName("Form/Image/Title/Caption/btnSysMenu");
    int iconRight   = FStyleSource.GetInt(sysMenuObj, "Left", 4) + FStyleSource.GetInt(sysMenuObj, "Width", 21);
    int capH = FStyleSource.GetInt(captionObj, "Height", 28);

    if (FTitleTextAlign == "taCenter") {
        int ctWidth = FStyleSource.GetInt(capTitle, "Width", 184);
        FTitleTextRect = {capX, capY,
                         ctWidth - FTitleTextML - FTitleTextMR, capH};
    } else {
        auto sysMenuObj = FStyleSource.GetObjectByName("Form/Image/Title/Caption/btnSysMenu");
        int smRight = FStyleSource.GetInt(sysMenuObj, "Left", 6) - 1 + FStyleSource.GetInt(sysMenuObj, "Width", 21);
        int ctTop   = FStyleSource.GetInt(capTitle, "Top", 2);
        int ctH     = FStyleSource.GetInt(capTitle, "Height", 28);
        FTitleTextRect = {smRight, ctTop, closeX - smRight, ctH};
    }

    // logo + icon
    FLogoTex = IMG_LoadTexture(FRenderer, (assetsDir + "/Logo" + logoNum + ".bmp").c_str());
    FIconTex = IMG_LoadTexture(FRenderer, (assetsDir + "/Icon" + iconNum + ".png").c_str());
    if (!FIconTex)
        FIconTex = IMG_LoadTexture(FRenderer, (assetsDir + "/Icon" + iconNum + ".ico").c_str());

    // music control bitmaps - replace black with clBtnFace
    auto wc = FStyleSource.GetSysColor("clBtnFace");
    auto loadBmp = [&](const std::string &path) -> SDL_Texture * {
        SDL_Surface *surf = SDL_LoadBMP(path.c_str());
        if (!surf) return nullptr;

        Uint32 black   = SDL_MapRGB(surf->format, 0, 0, 0);
        Uint32 replace = SDL_MapRGB(surf->format, wc.r, wc.g, wc.b);
        SDL_LockSurface(surf);

        auto *px = (Uint8 *)surf->pixels;
        for (int y = 0; y < surf->h; y++)
            for (int x = 0; x < surf->w; x++) {
                auto *p = (Uint32 *)(px + y * surf->pitch + x * surf->format->BytesPerPixel);
                if (*p == black) *p = replace;
            }

        SDL_UnlockSurface(surf);
        auto *tex = SDL_CreateTextureFromSurface(FRenderer, surf);
        SDL_FreeSurface(surf);

        return tex;
    };

    FPlayTex       = loadBmp(assetsDir + "/Play1.bmp");
    FPauseMusicTex = loadBmp(assetsDir + "/Pause1.bmp");
    FTrackBgTex    = loadBmp(assetsDir + "/TrackBkg.bmp");
    FTrackBtnTex   = loadBmp(assetsDir + "/TrackBtn1.bmp");

    // colors
    FClrBorder    = FStyleSource.GetColor("ktcBorder");
    FClrEdit      = FStyleSource.GetColor("ktcEdit");
    FClrLabel     = SDL_Color{0xe1, 0xe0, 0xe6, 255};
    FClrTitleText = FStyleSource.GetFontColor("ktfCaptionTextNormal");
    FClrEditText  = FStyleSource.GetFontColor("ktfEditBoxTextNormal");
    FClrBtnTextN  = FStyleSource.GetFontColor("ktfButtonTextNormal");
    FClrBtnTextH  = FStyleSource.GetFontColor("ktfButtonTextHot");
    FClrBtnTextP  = FStyleSource.GetFontColor("ktfButtonTextPressed");
    FClrBtnTextD  = FStyleSource.GetFontColor("ktfButtonTextDisabled");

    // init widgets
    DirEdit.Name = "DirEdit";
    DirEdit.Text = "/home/" + std::string(getenv("USER") ? getenv("USER") : "user") + "/Games/Example Game";
    GroupEdit.Name = "GroupEdit";
    GroupEdit.Text = "Example Game";
    Drives.push_back("/");

    btnLeftButton.Rect    = LayoutRect("btnLeftButton");    btnLeftButton.Caption  = "Exit";
    btnRightButton.Rect   = LayoutRect("btnRightButton");   btnRightButton.Caption = "Install";
    btnPause.Rect         = btnRightButton.Rect;            btnPause.Caption       = "Pause";    btnPause.Visible = false;
    btnDirBrowse.Rect     = LayoutRect("btnDirBrowse");     btnDirBrowse.Caption   = "Browse...";
    btnGroupBrowse.Rect   = LayoutRect("btnGroupBrowse");   btnGroupBrowse.Caption = "Browse...";

    LogLines.push_back("Waiting for Input...");

    // audio
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    FMusic = Mix_LoadMUS((assetsDir + "/Music" + musicNum + ".ogg").c_str());
    if (FMusic) Mix_PlayMusic(FMusic, -1);

    LastTick = SDL_GetTicks();
    return true;
}


SDL_Rect TWizardForm::LayoutRect(const std::string &name) const {
    if (!FLayout.contains(name)) return {0, 0, 0, 0};
    auto o = FLayout[name];
    return {FBorderL + o.value("x", 0), FTitleH + o.value("y", 0), o.value("w", 0), o.value("h", 0)};
}


void TWizardForm::Run() {
    while (FRunning) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) FRunning = false;
            else HandleEvent(e);
        }

        if (FStep == wpInstalling && !ISPaused) {
            Uint32 now = SDL_GetTicks();
            if (now - LastTick >= 50) {
                LastTick = now;
                ProgressValue += 5;

                if (ProgressValue % 100 == 0)
                    LogLines.push_back("Extracting file " + std::to_string(ProgressValue / 10) + " of 100...");
                if (ProgressValue > 1000)
                    CurPageChanged(wpFinished);
            }
        }

        SDL_SetRenderDrawColor(FRenderer, 0, 0, 0, 255);
        SDL_RenderClear(FRenderer);
        Paint();

        SDL_RenderPresent(FRenderer);
        SDL_Delay(1);
    }
}


void TWizardForm::Paint() {
    PaintNC();

    // logo
    if (FLogoTex) {
        SDL_Rect d = LayoutRect("WizardBitmapImage");
        SDL_RenderCopy(FRenderer, FLogoTex, nullptr, &d);
    }

    // bevels
    auto hl = FStyleSource.GetSysColor("clBtnHighlight");
    auto sh = FStyleSource.GetSysColor("clBtnShadow");

    DrawBevelRaised(FRenderer,  LayoutRect("bvlDirectories"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlDirInstall"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlIconGroup"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlOptions"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlInstallOptions"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlButtons"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlLeftButton"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlRightButton"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlProgressForm"), hl, sh);
    DrawBevelLowered(FRenderer, LayoutRect("bvlProgressGauge"), hl, sh);

    EditDraw();
    LabelDraw();
    CheckDraw();
    ButtonDraw();
    ProgressDraw();
    AudioDraw();
    ResultDraw();
}


void TWizardForm::PaintNC() {
    int w = FBorderL + ClientWidth + FBorderR;
    int h = FWindowH;

    TSeBitmapObject::DrawTex(FRenderer, FTitlebarTex, FTitleML, FTitleMT, FTitleMR, FTitleMB,
                            {0, 0, w, FTitleH});

    TSeBitmapObject::DrawTex(FRenderer, FBorderBTex, FBbML, FBbMT, FBbMR, FBbMB,
                            {0, h - FBorderB, w, FBorderB});

    int bH = h - FTitleH;
    TSeBitmapObject::DrawTex(FRenderer, FBorderLTex, FBlML, FBlMT, FBlMR, FBlMB,
                            {0, FTitleH, FBorderL, bH});
    TSeBitmapObject::DrawTex(FRenderer, FBorderRTex, FBrML, FBrMT, FBrMR, FBrMB,
                            {w - FBorderR, FTitleH, FBorderR, bH});

    // client fill
    auto bg = FStyleSource.GetSysColor("clBtnFace");
    SDL_SetRenderDrawColor(FRenderer, bg.r, bg.g, bg.b, 255);

    SDL_Rect client = {FBorderL, FTitleH, w - FBorderL - FBorderR, h - FTitleH - FBorderB};
    SDL_RenderFillRect(FRenderer, &client);

    // icon
    if (FIconTex) {
        auto sm = FStyleSource.GetObjectByName("Form/Image/Title/Caption/btnSysMenu");
        int smL = FStyleSource.GetInt(sm, "Left", 6) - 2, smT = FStyleSource.GetInt(sm, "Top", 2);
        int smW = FStyleSource.GetInt(sm, "Width", 21), smH = FStyleSource.GetInt(sm, "Height", 20);
        SDL_Rect id = {smL + (smW - 16) / 2, smT + (smH - 16) / 2, 16, 16};
        SDL_RenderCopy(FRenderer, FIconTex, nullptr, &id);
    }

    // title text
    TSeBitmapObject::DrawText(FRenderer, FStyleSource.CaptionFont(), FClrTitleText,
                              "Example Game", FTitleTextRect, FTitleTextAlign);

    // window buttons
    auto drawBtn = [&](const SDL_Rect &r, SDL_Texture *n, SDL_Texture *hot, bool hover) {
        auto *t = hover ? hot : n;
        if (!t) return;
        int tw, th; SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);
        SDL_Rect d = {r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2, tw, th};
        SDL_RenderCopy(FRenderer, t, nullptr, &d);
    };

    drawBtn(FCloseButtonRect, FWndCloseN, FWndCloseH, FCloseHover);
    drawBtn(FMinButtonRect,   FWndMinN,   FWndMinH,   FMinHover);
}


void TWizardForm::EditDraw() {
    auto *r = FRenderer;

    // dir edit
    SDL_Rect dirR = LayoutRect("DirEdit");
    TSeBitmapObject::DrawTex(r, FEditTex, FEdML, FEdMT, FEdMR, FEdMB, dirR);
    SDL_Rect inner = {dirR.x + FEdFrameML, dirR.y + FEdFrameMT,
                      dirR.w - FEdFrameML - FEdFrameMR, dirR.h - FEdFrameMT - FEdFrameMB};

    SDL_SetRenderDrawColor(r, FClrEdit.r, FClrEdit.g, FClrEdit.b, 255);
    SDL_RenderFillRect(r, &inner);

    SDL_Rect textR = {inner.x + 2, inner.y, inner.w - 4, inner.h};
    TSeBitmapObject::DrawText(r, FStyleSource.ButtonFont(), FClrEditText, DirEdit.Text, textR);

    if (DirEdit.Focused) {
        int cx = inner.x + 2;
        if (FStyleSource.ButtonFont()) {
            int tw, th;
            TTF_SizeUTF8(FStyleSource.ButtonFont(), DirEdit.Text.c_str(), &tw, &th);
            cx += std::min(tw, inner.w - 4);
        }

        SDL_SetRenderDrawColor(r, FClrEditText.r, FClrEditText.g, FClrEditText.b, 255);
        SDL_RenderDrawLine(r, cx, inner.y + 2, cx, inner.y + inner.h - 2);
    }

    // group edit
    SDL_Rect grR = LayoutRect("GroupEdit");
    TSeBitmapObject::DrawTex(r, FEditTex, FEdML, FEdMT, FEdMR, FEdMB, grR);
    SDL_Rect grInner = {grR.x + FEdFrameML, grR.y + FEdFrameMT,
                        grR.w - FEdFrameML - FEdFrameMR, grR.h - FEdFrameMT - FEdFrameMB};

    SDL_SetRenderDrawColor(r, FClrEdit.r, FClrEdit.g, FClrEdit.b, 255);
    SDL_RenderFillRect(r, &grInner);

    SDL_Rect grTextR = {grInner.x + 2, grInner.y, grInner.w - 4, grInner.h};
    TSeBitmapObject::DrawText(r, FStyleSource.ButtonFont(), FClrEditText, GroupEdit.Text, grTextR);

    // combobox
    SDL_Rect cbR = LayoutRect("cbxDrive");
    SDL_Rect btnR = {cbR.x + cbR.w - FEdFrameMR - ComboButtonWidth, cbR.y + FEdFrameMT,
                     ComboButtonWidth, cbR.h - FEdFrameMT - FEdFrameMB};

    TSeBitmapObject::DrawTex(r, FComboTex, FCbML, FCbMT, FCbMR, FCbMB, cbR);

    auto cbBg = FStyleSource.GetSysColor("clWindow");
    SDL_Rect cbFill = {cbR.x + 3, cbR.y + 3, btnR.x - 1 - (cbR.x + 3), cbR.h - 6};
    SDL_SetRenderDrawColor(r, cbBg.r, cbBg.g, cbBg.b, 255);
    SDL_RenderFillRect(r, &cbFill);

    std::string driveText = Drives.empty() ? "/" : Drives[ActiveDrive];
    SDL_Rect cbTextR = {cbFill.x + 2, cbFill.y, cbFill.w - 4, cbFill.h};
    TSeBitmapObject::DrawText(r, FStyleSource.ButtonFont(), FClrEditText, driveText, cbTextR);

    if (FComboBtnN) {
        TSeBitmapObject::DrawTex(r, FComboBtnN, FCbBtnML, FCbBtnMT, FCbBtnMR, FCbBtnMB, btnR);
        if (FComboArrowTex) {
            int aw, ah; SDL_QueryTexture(FComboArrowTex, nullptr, nullptr, &aw, &ah);
            SDL_Rect ad = {btnR.x + (btnR.w - aw) / 2, btnR.y + (btnR.h - ah) / 2, aw, ah};
            SDL_RenderCopy(r, FComboArrowTex, nullptr, &ad);
        }
    }
}


void TWizardForm::LabelDraw() {
    auto *f = FStyleSource.LabelFont();

    TSeBitmapObject::DrawText(FRenderer, f, FClrLabel, "Install directory",    LayoutRect("lblDirInstall"));
    TSeBitmapObject::DrawText(FRenderer, f, FClrLabel, "Create desktop shortcut", LayoutRect("lblDesktopIcon"));
    TSeBitmapObject::DrawText(FRenderer, f, FClrLabel, "Directory at Start Menu", LayoutRect("lblGroupDir"));
    TSeBitmapObject::DrawText(FRenderer, f, FClrLabel, "Create a Start Menu folder", LayoutRect("lblCreateGroup"));

    char buf[128];
    snprintf(buf, sizeof(buf), "At least %.2f GB of free space required", GameNeedSize / 1024.0);

    SDL_Rect diskR = LayoutRect("lblDiskSizeNeeded");
    SDL_Rect optR  = LayoutRect("bvlInstallOptions");

    diskR.w = optR.x + optR.w - diskR.x - 10;
    TSeBitmapObject::DrawText(FRenderer, f, FClrLabel, buf, diskR);

    TSeBitmapObject::DrawText(FRenderer, f, FClrLabel,
        "Do not create uninstaller and do not write any specific system info",
        LayoutRect("lblNoUninstaller"));
}


void TWizardForm::CheckDraw() {
    bool enabled = (FStep == wpSelectDir);

    auto draw = [&](const std::string &name, bool checked) {
        SDL_Rect r = LayoutRect(name);
        SDL_Texture *t = !enabled ? (checked ? FChkCheckedD : FChkUncheckedD)
                       :  checked ? FChkCheckedN : FChkUncheckedN;
        if (!t) return;

        int tw, th;
        SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);
        
        SDL_Rect dst = {r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2, tw, th};
        SDL_RenderCopy(FRenderer, t, nullptr, &dst);
    };

    draw("chbDesktopIcon",  chbCreateDesktopIcon);
    draw("chbCreateGroup",  chbCreateGroup);
    draw("chbNoUninstaller", chbNoUninstaller);
}


void TWizardForm::ButtonDraw() {
    auto draw = [&](const TButton &b) {
        if (!b.Visible) return;
        auto *t  = !b.Enabled ? FBtnDisabled : b.Pressed ? FBtnPressed : b.Hover ? FBtnHot : FBtnNormal;
        auto  tc = !b.Enabled ? FClrBtnTextD : b.Pressed ? FClrBtnTextP : b.Hover ? FClrBtnTextH : FClrBtnTextN;
        TSeBitmapObject::DrawTex(FRenderer, t, FBtnML, FBtnMT, FBtnMR, FBtnMB, b.Rect);
        TSeBitmapObject::DrawText(FRenderer, FStyleSource.ButtonFont(), tc, b.Caption, b.Rect, "taCenter");
    };

    draw(btnLeftButton);
    draw(btnRightButton);
    draw(btnPause);
    draw(btnDirBrowse);
    draw(btnGroupBrowse);
}


void TWizardForm::ProgressDraw() {
    SDL_Rect r = LayoutRect("ProgressGauge");
    TSeBitmapObject::DrawTex(FRenderer, FProgFrameTex, FPfML, FPfMT, FPfMR, FPfMB, r);

    if (ProgressValue > 0) {
        int barW = (r.w - FPfML - FPfMR) * ProgressValue / 1000;
        if (barW > 0) {
            SDL_Rect barR = {r.x + FPfML, r.y + FPfMT, barW, r.h - FPfMT - FPfMB};
            TSeBitmapObject::DrawTex(FRenderer, FProgBarTex, FPbML, FPbMT, FPbMR, FPbMB, barR);
        }
    }

    // log
    SDL_Rect logR = LayoutRect("memProgressLog");
    SDL_SetRenderDrawColor(FRenderer, FClrEdit.r, FClrEdit.g, FClrEdit.b, 255);
    SDL_RenderFillRect(FRenderer, &logR);
    SDL_SetRenderDrawColor(FRenderer, FClrBorder.r, FClrBorder.g, FClrBorder.b, 255);
    SDL_RenderDrawRect(FRenderer, &logR);

    auto *f = FStyleSource.ButtonFont();
    if (f) {
        int lineH = TTF_FontLineSkip(f);
        int maxLines  = (logR.h - 4) / lineH;
        int startLine = std::max(0, (int)LogLines.size() - maxLines);

        for (int i = startLine; i < (int)LogLines.size(); i++) {
            int y = logR.y + 2 + (i - startLine) * lineH;
            SDL_Rect tr = {logR.x + 4, y, logR.w - 8, lineH};
            TSeBitmapObject::DrawText(FRenderer, f, FClrEditText, LogLines[i], tr);
        }
    }
}


void TWizardForm::AudioDraw() {
    SDL_Rect playR  = LayoutRect("bmpPlayButton");
    SDL_Rect pauseR = LayoutRect("bmpPauseButton");
    if (FPlayTex)       SDL_RenderCopy(FRenderer, FPlayTex, nullptr, &playR);
    if (FPauseMusicTex) SDL_RenderCopy(FRenderer, FPauseMusicTex, nullptr, &pauseR);

    SDL_Rect trackR = LayoutRect("bmpTrackBar");
    if (FTrackBgTex) SDL_RenderCopy(FRenderer, FTrackBgTex, nullptr, &trackR);

    if (FTrackBtnTex) {
        int tw, th;
        SDL_QueryTexture(FTrackBtnTex, nullptr, nullptr, &tw, &th);

        int range  = trackR.w - tw;
        int thumbX = trackR.x + (int)(FVolume * range);
        int thumbY = trackR.y + (trackR.h - th) / 2;

        SDL_Rect tbR = {thumbX, thumbY, tw, th};
        SDL_RenderCopy(FRenderer, FTrackBtnTex, nullptr, &tbR);
    }
}


void TWizardForm::ResultDraw() {
    if (!ShowResult) return;

    SDL_Rect r = LayoutRect("lblInstallResult");
    SDL_Color c = {0x00, 0xdd, 0x34, 255};
    TSeBitmapObject::DrawText(FRenderer, FStyleSource.CaptionFont(), c, "Successfully Installed", r, "taCenter");
}


void TWizardForm::HandleEvent(const SDL_Event &e) {
    switch (e.type) {
        case SDL_MOUSEBUTTONDOWN: if (e.button.button == SDL_BUTTON_LEFT) WMMouseDown(e.button.x, e.button.y); break;
        case SDL_MOUSEBUTTONUP:   if (e.button.button == SDL_BUTTON_LEFT) WMMouseUp(e.button.x, e.button.y);   break;
        case SDL_MOUSEMOTION:     WMMouseMove(e.motion.x, e.motion.y); break;
        case SDL_TEXTINPUT:       WMTextInput(e.text.text); break;
        case SDL_KEYDOWN:         WMKeyDown(e.key.keysym.sym); break;
    }
}


void TWizardForm::WMMouseDown(int x, int y) {
    if (HitTest(x, y, FCloseButtonRect)) { FRunning = false; return; }
    if (HitTest(x, y, FMinButtonRect))   { SDL_MinimizeWindow(FWindow); return; }

    DirEdit.Focused    = HitTest(x, y, LayoutRect("DirEdit"));
    GroupEdit.Focused  = HitTest(x, y, LayoutRect("GroupEdit"));

    if (FStep == wpSelectDir) {
        if (HitTest(x, y, LayoutRect("chbDesktopIcon")) || HitTest(x, y, LayoutRect("lblDesktopIcon")))
            chbCreateDesktopIcon = !chbCreateDesktopIcon;

        if (HitTest(x, y, LayoutRect("chbCreateGroup")) || HitTest(x, y, LayoutRect("lblCreateGroup"))) {
            chbCreateGroup = !chbCreateGroup;
            GroupEdit.Enabled = chbCreateGroup;
        }

        if (HitTest(x, y, LayoutRect("chbNoUninstaller")) || HitTest(x, y, LayoutRect("lblNoUninstaller")))
            chbNoUninstaller = !chbNoUninstaller;
    }

    auto press = [&](TButton &b) { if (b.Visible && b.Enabled && HitTest(x, y, b.Rect)) b.Pressed = true; };
    press(btnLeftButton);  press(btnRightButton);
    press(btnPause);       press(btnDirBrowse);  press(btnGroupBrowse);

    if (HitTest(x, y, LayoutRect("bmpPlayButton"))) {
        if (FMusic && !Mix_PlayingMusic()) Mix_PlayMusic(FMusic, -1);
        else if (Mix_PausedMusic()) Mix_ResumeMusic();
    }

    if (HitTest(x, y, LayoutRect("bmpPauseButton"))) {
        if (Mix_PlayingMusic() && !Mix_PausedMusic()) Mix_PauseMusic();
    }

    SDL_Rect trackR = LayoutRect("bmpTrackBar");
    SDL_Rect volHit = {trackR.x - 4, trackR.y - 6, trackR.w + 8, trackR.h + 12};
    if (HitTest(x, y, volHit)) {
        FDraggingVolume = true;
        FVolume = std::max(0.0f, std::min(1.0f, float(x - trackR.x) / trackR.w));
        Mix_VolumeMusic(int(FVolume * MIX_MAX_VOLUME));
    }

    // drag by titlebar + logo area
    if (y < FTitleH + 65 && !HitTest(x, y, FCloseButtonRect) && !HitTest(x, y, FMinButtonRect)) {
        FDragging = true;
        FDragX = x; FDragY = y;
    }
}


void TWizardForm::WMMouseUp(int x, int y) {
    FDragging = false;
    FDraggingVolume = false;

    if (btnLeftButton.Pressed) {
        btnLeftButton.Pressed = false;
        if (HitTest(x, y, btnLeftButton.Rect)) {
            if (FStep == wpInstalling) { CurPageChanged(wpSelectDir); LogLines.push_back("Cancelled."); }
            else if (FStep == wpFinished) { /* run */ }
            else FRunning = false;
        }
    }

    if (btnRightButton.Pressed) {
        btnRightButton.Pressed = false;
        if (HitTest(x, y, btnRightButton.Rect)) {
            if (FStep == wpSelectDir) CurPageChanged(wpInstalling);
            else if (FStep == wpFinished) FRunning = false;
        }
    }

    if (btnPause.Pressed) {
        btnPause.Pressed = false;
        if (HitTest(x, y, btnPause.Rect)) {
            ISPaused = !ISPaused;
            btnPause.Caption = ISPaused ? "Resume" : "Pause";
        }
    }
}


void TWizardForm::WMMouseMove(int x, int y) {
    FCloseHover = HitTest(x, y, FCloseButtonRect);
    FMinHover   = HitTest(x, y, FMinButtonRect);

    auto hover = [&](TButton &b) { b.Hover = b.Visible && b.Enabled && HitTest(x, y, b.Rect); };
    hover(btnLeftButton);  hover(btnRightButton);
    hover(btnPause);       hover(btnDirBrowse);  hover(btnGroupBrowse);

    if (FDraggingVolume) {
        SDL_Rect trackR = LayoutRect("bmpTrackBar");
        FVolume = std::max(0.0f, std::min(1.0f, float(x - trackR.x) / trackR.w));
        Mix_VolumeMusic(int(FVolume * MIX_MAX_VOLUME));
    }

    if (FDragging && !FDraggingVolume) {
        int wx, wy;
        SDL_GetWindowPosition(FWindow, &wx, &wy);
        SDL_SetWindowPosition(FWindow, wx + x - FDragX, wy + y - FDragY);
    }
}


void TWizardForm::WMTextInput(const char *text) {
    if (DirEdit.Focused && DirEdit.Enabled) DirEdit.Text += text;
    else if (GroupEdit.Focused && GroupEdit.Enabled) GroupEdit.Text += text;
}


void TWizardForm::WMKeyDown(SDL_Keycode key) {
    TNewEdit *active = nullptr;
    if (DirEdit.Focused)       active = &DirEdit;
    else if (GroupEdit.Focused) active = &GroupEdit;
    if (!active || !active->Enabled) return;

    if (key == SDLK_BACKSPACE && !active->Text.empty())
        active->Text.pop_back();
}


void TWizardForm::CurPageChanged(ISStep step) {
    FStep = step;

    if (step == wpSelectDir) {
        DirEdit.Enabled   = true;
        GroupEdit.Enabled  = chbCreateGroup;

        btnLeftButton  = {LayoutRect("btnLeftButton"),  "Exit",    false, false, true, true};
        btnRightButton = {LayoutRect("btnRightButton"), "Install", false, false, true, true};
        btnPause.Visible = false;

        ShowResult = false;

    } else if (step == wpInstalling) {
        DirEdit.Enabled  = false;  DirEdit.Focused  = false;
        GroupEdit.Enabled = false;  GroupEdit.Focused = false;

        btnLeftButton  = {LayoutRect("btnLeftButton"),  "Cancel", false, false, true, true};
        btnRightButton.Visible = false;
        btnPause = {LayoutRect("btnRightButton"), "Pause", false, false, true, true};

        ProgressValue = 0;
        ISPaused = false;

        LogLines.clear();
        LogLines.push_back("Extracting files...");
        LastTick = SDL_GetTicks();

    } else if (step == wpFinished) {
        btnPause.Visible = false;
        btnLeftButton  = {LayoutRect("btnLeftButton"),  "Run",    false, false, true, true};
        btnRightButton = {LayoutRect("btnRightButton"), "Finish", false, false, true, true};

        ShowResult = true;
        LogLines.push_back("Done!");
    }
}


void TWizardForm::DeinitializeSetup() {
    if (FMusic) { Mix_FreeMusic(FMusic); FMusic = nullptr; }
    Mix_CloseAudio();

    if (FLogoTex)       SDL_DestroyTexture(FLogoTex);
    if (FIconTex)       SDL_DestroyTexture(FIconTex);
    if (FPlayTex)       SDL_DestroyTexture(FPlayTex);
    if (FPauseMusicTex) SDL_DestroyTexture(FPauseMusicTex);
    if (FTrackBgTex)    SDL_DestroyTexture(FTrackBgTex);
    if (FTrackBtnTex)   SDL_DestroyTexture(FTrackBtnTex);
    if (FRenderer)      SDL_DestroyRenderer(FRenderer);
    if (FWindow)        SDL_DestroyWindow(FWindow);
}
