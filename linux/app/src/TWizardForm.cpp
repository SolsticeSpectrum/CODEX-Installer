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
    FComboBtnP = load("combobox_button_pressed.png");
    FComboBtnD = load("combobox_button_disabled.png");

    // scrollbar
    auto svf = FStyleSource.GetObjectByName("ScrollBar/VertFrame");
    FStyleSource.GetMargins(svf, FSvfML, FSvfMT, FSvfMR, FSvfMB);
    FScrollVfTex = load("scrollbar_vert_frame_normal.png");

    auto sbBtn = FStyleSource.GetObjectByName("ScrollBar/TopButton");
    FStyleSource.GetMargins(sbBtn, FSbBtnML, FSbBtnMT, FSbBtnMR, FSbBtnMB);
    FScrollTopN      = load("scrollbar_top_btn_normal.png");
    FScrollTopH      = load("scrollbar_top_btn_hot.png");
    FScrollTopP      = load("scrollbar_top_btn_pressed.png");
    FScrollTopD      = load("scrollbar_top_btn_disabled.png");
    FScrollBotN      = load("scrollbar_bottom_btn_normal.png");
    FScrollBotH      = load("scrollbar_bottom_btn_hot.png");
    FScrollBotP      = load("scrollbar_bottom_btn_pressed.png");
    FScrollBotD      = load("scrollbar_bottom_btn_disabled.png");
    FScrollTopArrowN = load("scrollbar_top_btn_arrow_normal.png");
    FScrollTopArrowD = load("scrollbar_top_btn_arrow_disabled.png");
    FScrollBotArrowN = load("scrollbar_bottom_btn_arrow_normal.png");
    FScrollBotArrowD = load("scrollbar_bottom_btn_arrow_disabled.png");

    auto svSlider = FStyleSource.GetObjectByName("ScrollBar/VertSlider");
    FStyleSource.GetMargins(svSlider, FStML, FStMT, FStMR, FStMB);
    FScrollThumbN = load("scrollbar_vert_slider_normal.png");
    FScrollThumbH = load("scrollbar_vert_slider_hot.png");
    FScrollThumbP = load("scrollbar_vert_slider_pressed.png");

    // pre-multiplied alpha blend: src + dst*(1-srcA)
    auto pmBlend = SDL_ComposeCustomBlendMode(
        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD,
        SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE_MINUS_SRC_ALPHA, SDL_BLENDOPERATION_ADD);
    if (FScrollTopArrowN) SDL_SetTextureBlendMode(FScrollTopArrowN, pmBlend);
    if (FScrollTopArrowD) SDL_SetTextureBlendMode(FScrollTopArrowD, pmBlend);
    if (FScrollBotArrowN) SDL_SetTextureBlendMode(FScrollBotArrowN, pmBlend);
    if (FScrollBotArrowD) SDL_SetTextureBlendMode(FScrollBotArrowD, pmBlend);

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
    int smLeft      = FStyleSource.GetInt(sysMenuObj, "Left", 6);
    int smWidth     = FStyleSource.GetInt(sysMenuObj, "Width", 21);

    int ctH  = FStyleSource.GetInt(capTitle, "Height", 28);
    int capH = FStyleSource.GetInt(captionObj, "Height", 28);

    int textY = (capY + ctH >= FTitleH) ? capY : 0;
    int textH = (capY + ctH >= FTitleH) ? ctH  : FTitleH;

    if (FTitleTextAlign == "taCenter") {
        int ctWidth = FStyleSource.GetInt(capTitle, "Width", 184);
        FTitleTextRect = {capX, textY,
                         ctWidth - FTitleTextML - FTitleTextMR, textH};
    } else {
        int smRight = smLeft - 1 + smWidth;
        FTitleTextRect = {smRight, textY, closeX - smRight, textH};
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

    FPlayN      = loadBmp(assetsDir + "/Play1.bmp");
    FPlayH      = loadBmp(assetsDir + "/Play2.bmp");
    FPlayP      = loadBmp(assetsDir + "/Play3.bmp");
    FPauseN     = loadBmp(assetsDir + "/Pause1.bmp");
    FPauseH     = loadBmp(assetsDir + "/Pause2.bmp");
    FPauseP     = loadBmp(assetsDir + "/Pause3.bmp");
    FTrackBgTex = loadBmp(assetsDir + "/TrackBkg.bmp");
    FTrackBtnN  = loadBmp(assetsDir + "/TrackBtn1.bmp");
    FTrackBtnH  = loadBmp(assetsDir + "/TrackBtn2.bmp");
    FTrackBtnP  = loadBmp(assetsDir + "/TrackBtn3.bmp");

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

                if (ProgressValue % 10 == 0)
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

    auto *cbBtnTex = FComboPressed ? FComboBtnP : (FComboHover ? FComboBtnH : FComboBtnN);
    if (cbBtnTex) {
        TSeBitmapObject::DrawTex(r, cbBtnTex, FCbBtnML, FCbBtnMT, FCbBtnMR, FCbBtnMB, btnR);
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

    auto draw = [&](const std::string &name, bool checked, bool hover) {
        SDL_Rect r = LayoutRect(name);
        SDL_Texture *t;
        if (!enabled)   t = checked ? FChkCheckedD : FChkUncheckedD;
        else if (hover) t = checked ? FChkCheckedH : FChkUncheckedH;
        else            t = checked ? FChkCheckedN : FChkUncheckedN;
        if (!t) return;

        int tw, th;
        SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);

        SDL_Rect dst = {r.x + (r.w - tw) / 2, r.y + (r.h - th) / 2, tw, th};
        SDL_RenderCopy(FRenderer, t, nullptr, &dst);
    };

    draw("chbDesktopIcon",   chbCreateDesktopIcon,  chbDesktopIconHover);
    draw("chbCreateGroup",   chbCreateGroup,         chbCreateGroupHover);
    draw("chbNoUninstaller", chbNoUninstaller,       chbNoUninstallerHover);
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
        int barW = (r.w - 2) * ProgressValue / 1000;
        if (barW > 0) {
            SDL_Rect barR = {r.x + 1, r.y + 1, barW, r.h - 2};
            TSeBitmapObject::DrawTex(FRenderer, FProgBarTex, FPbML, FPbMT, FPbMR, FPbMB, barR);
        }
    }

    // log
    SDL_Rect logR = LayoutRect("memProgressLog");
    SDL_Rect logInner = {logR.x + FEdFrameML, logR.y + FEdFrameMT,
                         logR.w - FEdFrameML - FEdFrameMR, logR.h - FEdFrameMT - FEdFrameMB};

    int scrollW  = 17;
    int scrollX  = logInner.x + logInner.w - scrollW;
    SDL_Rect upR = {scrollX, logInner.y, scrollW, scrollW};
    SDL_Rect dnR = {scrollX, logInner.y + logInner.h - scrollW, scrollW, scrollW};

    TSeBitmapObject::DrawTex(FRenderer, FEditTex, FEdML, FEdMT, FEdMR, FEdMB, logR);

    auto logBg = FStyleSource.GetSysColor("clWindow");
    SDL_SetRenderDrawColor(FRenderer, logBg.r, logBg.g, logBg.b, 255);
    SDL_RenderFillRect(FRenderer, &logInner);

    // VertFrame track clipped to scrollbar column
    SDL_Rect vfR   = {scrollX - FSvfML, upR.y + upR.h,
                      scrollW + FSvfML + FSvfMR, dnR.y - (upR.y + upR.h)};
    SDL_Rect clipR = {scrollX, upR.y + upR.h, scrollW, dnR.y - (upR.y + upR.h)};

    SDL_RenderSetClipRect(FRenderer, &clipR);

    TSeBitmapObject::DrawTex(FRenderer, FScrollVfTex, FSvfML, FSvfMT, FSvfMR, FSvfMB, vfR);
    SDL_RenderSetClipRect(FRenderer, nullptr);

    SDL_Rect textArea = {logInner.x + 1, logInner.y + 1,
                         logInner.w - scrollW - 2, logInner.h - 2};
    SDL_SetRenderDrawColor(FRenderer, FClrEdit.r, FClrEdit.g, FClrEdit.b, 255);
    SDL_RenderFillRect(FRenderer, &textArea);

    // scrollbar state
    auto *logFont = FStyleSource.ButtonFont();
    int lineH     = logFont ? TTF_FontLineSkip(logFont) : 14;
    int maxLines  = (textArea.h - 4) / lineH;
    int total     = (int)LogLines.size();
    bool disabled = total <= maxLines;
    int trackTop  = upR.y + upR.h;
    int trackH    = dnR.y - trackTop;

    int maxScroll = std::max(0, total - maxLines);
    if (maxScroll > FLogPrevMax && FLogScroll >= FLogPrevMax && !FLogDragging)
        FLogScroll = maxScroll;
    FLogPrevMax = maxScroll;
    if (FLogScroll > maxScroll) FLogScroll = maxScroll;
    if (FLogScroll < 0) FLogScroll = 0;

    FLogUpR = upR; FLogDnR = dnR;
    FLogTrackTop = trackTop; FLogTrackH = trackH;
    FLogMaxScroll = maxScroll; FLogMaxLines = maxLines;

    // buttons
    auto *topBtn   = disabled ? FScrollTopD      : (FScrollTopPressed ? FScrollTopP : (FScrollTopHover ? FScrollTopH : FScrollTopN));
    auto *botBtn   = disabled ? FScrollBotD      : (FScrollBotPressed ? FScrollBotP : (FScrollBotHover ? FScrollBotH : FScrollBotN));
    auto *topArrow = disabled ? FScrollTopArrowD : FScrollTopArrowN;
    auto *botArrow = disabled ? FScrollBotArrowD : FScrollBotArrowN;

    TSeBitmapObject::DrawTex(FRenderer, topBtn, FSbBtnML, FSbBtnMT, FSbBtnMR, FSbBtnMB, upR);
    TSeBitmapObject::DrawTex(FRenderer, botBtn, FSbBtnML, FSbBtnMT, FSbBtnMR, FSbBtnMB, dnR);

    if (topArrow) {
        int aw, ah; SDL_QueryTexture(topArrow, nullptr, nullptr, &aw, &ah);
        SDL_Rect d = {upR.x + (upR.w - aw) / 2, upR.y + (upR.h - ah) / 2, aw, ah};
        SDL_RenderCopy(FRenderer, topArrow, nullptr, &d);
    }
    if (botArrow) {
        int aw, ah; SDL_QueryTexture(botArrow, nullptr, nullptr, &aw, &ah);
        SDL_Rect d = {dnR.x + (dnR.w - aw) / 2, dnR.y + (dnR.h - ah) / 2, aw, ah};
        SDL_RenderCopy(FRenderer, botArrow, nullptr, &d);
    }

    // thumb
    FLogThumbR = {};
    if (!disabled && trackH > 0) {
        int thumbH = std::max(FStMT + FStMB + 4, trackH * maxLines / total);
        int thumbY = trackTop;
        if (maxScroll > 0)
            thumbY += (trackH - thumbH) * FLogScroll / maxScroll;

        FLogThumbR = {scrollX, thumbY, scrollW, thumbH};
        auto *tex = FLogDragging ? FScrollThumbP : (FLogThumbHover ? FScrollThumbH : FScrollThumbN);
        TSeBitmapObject::DrawTex(FRenderer, tex, FStML, FStMT, FStMR, FStMB, FLogThumbR);
    }

    // text
    if (logFont) {
        for (int i = 0; i < maxLines && (FLogScroll + i) < total; i++) {
            int y = textArea.y + 2 + i * lineH;
            SDL_Rect tr = {textArea.x + 2, y, textArea.w - 4, lineH};
            TSeBitmapObject::DrawText(FRenderer, logFont, FClrEditText, LogLines[FLogScroll + i], tr);
        }
    }
}


void TWizardForm::AudioDraw() {
    SDL_Rect playR  = LayoutRect("bmpPlayButton");
    SDL_Rect pauseR = LayoutRect("bmpPauseButton");

    auto *playTex  = FPlayPressed  ? FPlayP  : (FPlayHover  ? FPlayH  : FPlayN);
    auto *pauseTex = FPausePressed ? FPauseP : (FPauseHover ? FPauseH : FPauseN);
    if (playTex)  SDL_RenderCopy(FRenderer, playTex, nullptr, &playR);
    if (pauseTex) SDL_RenderCopy(FRenderer, pauseTex, nullptr, &pauseR);

    SDL_Rect trackR = LayoutRect("bmpTrackBar");
    if (FTrackBgTex) SDL_RenderCopy(FRenderer, FTrackBgTex, nullptr, &trackR);

    auto *trkTex = FDraggingVolume ? FTrackBtnP : (FTrackBtnHover ? FTrackBtnH : FTrackBtnN);
    if (trkTex) {
        int tw, th;
        SDL_QueryTexture(trkTex, nullptr, nullptr, &tw, &th);

        int range  = trackR.w - tw;
        int thumbX = trackR.x + (int)(FVolume * range);
        int thumbY = trackR.y + (trackR.h - th) / 2;

        SDL_Rect tbR = {thumbX, thumbY, tw, th};
        SDL_RenderCopy(FRenderer, trkTex, nullptr, &tbR);
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
        case SDL_MOUSEWHEEL:
            if (FLogMaxScroll > 0) {
                FLogScroll -= e.wheel.y * 3;
                if (FLogScroll < 0) FLogScroll = 0;
                if (FLogScroll > FLogMaxScroll) FLogScroll = FLogMaxScroll;
            }
            break;
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

    if (HitTest(x, y, LayoutRect("cbxDrive"))) {
        FComboPressed = true;
        if (!Drives.empty()) ActiveDrive = (ActiveDrive + 1) % (int)Drives.size();
    }

    if (HitTest(x, y, LayoutRect("bmpPlayButton"))) {
        FPlayPressed = true;
        if (FMusic && !Mix_PlayingMusic()) Mix_PlayMusic(FMusic, -1);
        else if (Mix_PausedMusic()) Mix_ResumeMusic();
    }

    if (HitTest(x, y, LayoutRect("bmpPauseButton"))) {
        FPausePressed = true;
        if (Mix_PlayingMusic() && !Mix_PausedMusic()) Mix_PauseMusic();
    }

    SDL_Rect trackR = LayoutRect("bmpTrackBar");
    SDL_Rect volHit = {trackR.x - 4, trackR.y - 6, trackR.w + 8, trackR.h + 12};
    if (HitTest(x, y, volHit)) {
        FDraggingVolume = true;
        FVolume = std::max(0.0f, std::min(1.0f, float(x - trackR.x) / trackR.w));
        Mix_VolumeMusic(int(FVolume * MIX_MAX_VOLUME));
    }

    // scrollbar arrows
    if (FLogMaxScroll > 0) {
        if (HitTest(x, y, FLogUpR)) { FScrollTopPressed = true; FLogScroll = std::max(0, FLogScroll - 1); return; }
        if (HitTest(x, y, FLogDnR)) { FScrollBotPressed = true; FLogScroll = std::min(FLogMaxScroll, FLogScroll + 1); return; }
        if (FLogThumbR.h > 0 && HitTest(x, y, FLogThumbR)) {
            FLogDragging  = true;
            FLogDragY     = y;
            FLogDragStart = FLogScroll;
            return;
        }
    }

    // drag by titlebar + logo area
    if (y < FTitleH + 65 && !HitTest(x, y, FCloseButtonRect) && !HitTest(x, y, FMinButtonRect)) {
        FDragging = true;
        FDragX = x; FDragY = y;
    }
}


void TWizardForm::WMMouseUp(int x, int y) {
    FDragging         = false;
    FDraggingVolume   = false;
    FLogDragging      = false;
    FPlayPressed      = false;
    FPausePressed     = false;
    FScrollTopPressed = false;
    FScrollBotPressed = false;
    FComboPressed     = false;

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

    chbDesktopIconHover   = HitTest(x, y, LayoutRect("chbDesktopIcon"))  || HitTest(x, y, LayoutRect("lblDesktopIcon"));
    chbCreateGroupHover   = HitTest(x, y, LayoutRect("chbCreateGroup"))  || HitTest(x, y, LayoutRect("lblCreateGroup"));
    chbNoUninstallerHover = HitTest(x, y, LayoutRect("chbNoUninstaller"))|| HitTest(x, y, LayoutRect("lblNoUninstaller"));

    FPlayHover       = HitTest(x, y, LayoutRect("bmpPlayButton"));
    FPauseHover      = HitTest(x, y, LayoutRect("bmpPauseButton"));
    FComboHover      = HitTest(x, y, LayoutRect("cbxDrive"));
    FLogThumbHover   = FLogThumbR.h > 0 && HitTest(x, y, FLogThumbR);
    FScrollTopHover  = HitTest(x, y, FLogUpR);
    FScrollBotHover  = HitTest(x, y, FLogDnR);

    if (FTrackBtnN) {
        int tw, th; SDL_QueryTexture(FTrackBtnN, nullptr, nullptr, &tw, &th);
        SDL_Rect trackR = LayoutRect("bmpTrackBar");
        int thumbX = trackR.x + (int)(FVolume * (trackR.w - tw));
        int thumbY = trackR.y + (trackR.h - th) / 2;
        SDL_Rect tbR = {thumbX, thumbY, tw, th};
        FTrackBtnHover = HitTest(x, y, tbR);
    }

    if (FDraggingVolume) {
        SDL_Rect trackR = LayoutRect("bmpTrackBar");
        FVolume = std::max(0.0f, std::min(1.0f, float(x - trackR.x) / trackR.w));
        Mix_VolumeMusic(int(FVolume * MIX_MAX_VOLUME));
    }

    if (FLogDragging && FLogTrackH > 0 && FLogThumbR.h > 0) {
        int range = FLogTrackH - FLogThumbR.h;
        if (range > 0) {
            int dy = y - FLogDragY;
            FLogScroll = FLogDragStart + dy * FLogMaxScroll / range;
            if (FLogScroll < 0) FLogScroll = 0;
            if (FLogScroll > FLogMaxScroll) FLogScroll = FLogMaxScroll;
        }
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
    if (FPlayN)         SDL_DestroyTexture(FPlayN);
    if (FPlayH)         SDL_DestroyTexture(FPlayH);
    if (FPlayP)         SDL_DestroyTexture(FPlayP);
    if (FPauseN)        SDL_DestroyTexture(FPauseN);
    if (FPauseH)        SDL_DestroyTexture(FPauseH);
    if (FPauseP)        SDL_DestroyTexture(FPauseP);
    if (FTrackBgTex)    SDL_DestroyTexture(FTrackBgTex);
    if (FTrackBtnN)     SDL_DestroyTexture(FTrackBtnN);
    if (FTrackBtnH)     SDL_DestroyTexture(FTrackBtnH);
    if (FTrackBtnP)     SDL_DestroyTexture(FTrackBtnP);
    if (FScrollVfTex)   SDL_DestroyTexture(FScrollVfTex);
    if (FRenderer)      SDL_DestroyRenderer(FRenderer);
    if (FWindow)        SDL_DestroyWindow(FWindow);
}
