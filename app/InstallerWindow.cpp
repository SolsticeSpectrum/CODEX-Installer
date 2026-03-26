#include "InstallerWindow.h"
#include <fstream>
#include <filesystem>
#include <cstring>

static const int CLIENT_W = 482;
static const int CLIENT_H = 583;

// =============================================================================
// Init
// =============================================================================

bool InstallerWindow::init(const std::string &assetsDir, const std::string &themeDir,
                           const std::string &layoutPath, const std::string &fontsDir,
                           const std::string &logoNum, const std::string &iconNum,
                           const std::string &musicNum) {
    m_assetsDir = assetsDir;

    // Load layout.json
    std::ifstream lf(layoutPath);
    if (lf.is_open()) m_layout = json::parse(lf, nullptr, false);

    // Read theme.json directly first to get frame dimensions
    {
        std::ifstream tf(themeDir + "/theme.json");
        if (!tf.is_open()) { SDL_Log("Cannot open theme.json"); return false; }
        json theme = json::parse(tf, nullptr, false);
        if (theme.is_discarded()) { SDL_Log("Invalid theme.json"); return false; }
        auto objs = theme["objects"];
        // Find Form/Image children for dimensions
        for (const auto &o : objs) {
            if (o.value("_name","") != "Form") continue;
            if (!o.contains("_children")) break;
            for (const auto &c : o["_children"]) {
                if (c.value("_name","") != "Image") continue;
                if (!c.contains("_children")) break;
                for (const auto &gc : c["_children"]) {
                    std::string n = gc.value("_name","");
                    if (n == "Title") m_titleH = gc.value("Height", 30);
                    else if (n == "LeftBorder") m_borderL = gc.value("Width", 8);
                    else if (n == "RightBorder") m_borderR = gc.value("Width", 8);
                    else if (n == "BottomBorder") m_borderB = gc.value("Height", 8);
                }
                break;
            }
            break;
        }
    }

    // Window size = client area + borders + title bar
    int winW = m_borderL + CLIENT_W + m_borderR;
    int winH = 618; // all variants are 618px tall in the original

    // Create SDL window
    m_window = SDL_CreateWindow("Example Game",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winW, winH, SDL_WINDOW_BORDERLESS | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!m_window) { SDL_Log("Cannot create window"); return false; }

    m_renderer = SDL_CreateRenderer(m_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!m_renderer) { SDL_Log("Cannot create renderer"); return false; }

    // Load VCL renderer with the SDL renderer
    if (!m_vcl.load(m_renderer, themeDir + "/theme.json", themeDir + "/assets", fontsDir)) {
        SDL_Log("Cannot load VCL renderer");
        return false;
    }

    // Enable SDL text input
    SDL_StartTextInput();

    // --- Load all assets from theme (margins from style objects) ---
    auto loadTex = [&](const std::string &f) { return m_vcl.loadTexture(f); };

    // Title bar
    auto titleObj = m_vcl.findObject("Form/Image/Title");
    m_vcl.getMargins(titleObj, m_titleML, m_titleMT, m_titleMR, m_titleMB);
    m_titlebarTex = loadTex("titlebar_active.png");

    // Borders
    auto lbObj = m_vcl.findObject("Form/Image/LeftBorder");
    m_vcl.getMargins(lbObj, m_blML, m_blMT, m_blMR, m_blMB);
    m_borderLTex = loadTex("border_leftborder_active.png");
    auto rbObj = m_vcl.findObject("Form/Image/RightBorder");
    m_vcl.getMargins(rbObj, m_brML, m_brMT, m_brMR, m_brMB);
    m_borderRTex = loadTex("border_rightborder_active.png");
    auto bbObj = m_vcl.findObject("Form/Image/BottomBorder");
    m_vcl.getMargins(bbObj, m_bbML, m_bbMT, m_bbMR, m_bbMB);
    m_borderBTex = loadTex("border_bottomborder_active.png");

    // Border sizes from actual bitmap dimensions
    if (m_borderLTex) { int tw; SDL_QueryTexture(m_borderLTex, nullptr, nullptr, &tw, nullptr); m_borderL = tw; }
    if (m_borderRTex) { int tw; SDL_QueryTexture(m_borderRTex, nullptr, nullptr, &tw, nullptr); m_borderR = tw; }
    if (m_borderBTex) { int th; SDL_QueryTexture(m_borderBTex, nullptr, nullptr, nullptr, &th); m_borderB = th; }

    // Resize window width only (height is fixed at 618)
    winW = m_borderL + CLIENT_W + m_borderR;
    SDL_SetWindowSize(m_window, winW, 618);

    // Client background
    auto clientObj = m_vcl.findObject("Form/Image/Client");
    m_clientBgTex = loadTex("form_client.png");
    m_clientTile = m_vcl.getString(clientObj, "TileStyle", "tsTile");

    // Button face
    auto btnFace = m_vcl.findObject("Button/Face");
    m_vcl.getMargins(btnFace, m_btnML, m_btnMT, m_btnMR, m_btnMB);
    m_btnNTex = loadTex("button_normal.png");
    m_btnHTex = loadTex("button_hot.png");
    m_btnPTex = loadTex("button_pressed.png");
    m_btnDTex = loadTex("button_disabled.png");

    // Edit frame: bitmap has 9-patch margins, Frame has client inset margins
    auto editFrameObj = m_vcl.findObject("Edit/Frame");
    m_vcl.getMargins(editFrameObj, m_edFML, m_edFMT, m_edFMR, m_edFMB);
    auto editBitmapObj = m_vcl.findObject("Edit/Frame/bitmap");
    m_vcl.getMargins(editBitmapObj, m_edML, m_edMT, m_edMR, m_edMB);
    m_editTex = loadTex("edit_frame.png");

    // ComboBox frame
    auto cbFrame = m_vcl.findObject("ComboBox/Frame/bitmap");
    m_vcl.getMargins(cbFrame, m_cbML, m_cbMT, m_cbMR, m_cbMB);
    m_comboTex = loadTex("combobox_frame.png");
    m_comboArrowTex = loadTex("combobox_arrow_normal.png");
    auto comboBtnObj = m_vcl.findObject("ComboBox/Button");
    m_vcl.getMargins(comboBtnObj, m_cbBtnML, m_cbBtnMT, m_cbBtnMR, m_cbBtnMB);
    m_cbBtnW = m_vcl.getInt(comboBtnObj, "Width", 17);
    m_comboBtnN = loadTex("combobox_button_normal.png");
    m_comboBtnH = loadTex("combobox_button_hot.png");

    // ProgressBar
    auto progFrame = m_vcl.findObject("ProgressBar/Frame");
    m_vcl.getMargins(progFrame, m_pfML, m_pfMT, m_pfMR, m_pfMB);
    m_progFrameTex = loadTex("progressbar_frame.png");
    auto progBar = m_vcl.findObject("ProgressBar/BarHorz");
    m_vcl.getMargins(progBar, m_pbML, m_pbMT, m_pbMR, m_pbMB);
    m_progBarTex = loadTex("progressbar_bar.png");

    // Checkboxes
    m_chkUN = loadTex("checkbox_unchecked_normal.png");
    m_chkUH = loadTex("checkbox_unchecked_hot.png");
    m_chkUD = loadTex("checkbox_unchecked_disabled.png");
    m_chkCN = loadTex("checkbox_checked_normal.png");
    m_chkCH = loadTex("checkbox_checked_hot.png");
    m_chkCD = loadTex("checkbox_checked_disabled.png");

    // Window buttons
    auto btnCloseObj = m_vcl.findObject("Form/Image/Title/Caption/sysButtons/btnClose");
    auto btnMinObj = m_vcl.findObject("Form/Image/Title/Caption/sysButtons/btnMin");
    m_wbW = m_vcl.getInt(btnCloseObj, "Width", 20);
    m_wbH = m_vcl.getInt(btnCloseObj, "Height", 28);
    m_wbY = m_vcl.getInt(btnCloseObj, "Top", 2);
    m_wbCloseN = loadTex("wnd_btnClose_normal.png");
    m_wbCloseH = loadTex("wnd_btnClose_hot.png");
    m_wbMaxN = loadTex("wnd_btnMax_normal.png");
    m_wbMaxH = loadTex("wnd_btnMax_hot.png");
    m_wbResN = loadTex("wnd_btnRes_normal.png");
    m_wbResH = loadTex("wnd_btnRes_hot.png");
    m_wbMinN = loadTex("wnd_btnMin_normal.png");
    m_wbMinH = loadTex("wnd_btnMin_hot.png");
    m_wbHelpN = loadTex("wnd_btnHelp_normal.png");
    m_wbHelpH = loadTex("wnd_btnHelp_hot.png");

    // Simulate VCL Aligning() for the title bar button positions:
    // Caption: saTopRight in Title → x = winW - Caption.Width
    auto captionObj = m_vcl.findObject("Form/Image/Title/Caption");
    int capW = m_vcl.getInt(captionObj, "Width", 300);
    int capX = winW - capW;
    int capY = m_vcl.getInt(captionObj, "Top", 2);

    // CaptionLeft: saLeft inside Caption, Width=9
    auto capLeftObj = m_vcl.findChild(captionObj, "CaptionLeft");
    int capLeftW = m_vcl.getInt(capLeftObj, "Width", 9);

    // sysButtons: saClient inside Caption → starts after CaptionLeft
    int sysX = capX + capLeftW;
    int sysW = capW - capLeftW;

    // Inside sysButtons, VCL processes saMostRight first, then saRight, then saClient:
    // CaptionRight: saMostRight, Width=7 (margin between buttons and right border)
    auto capRightObj = m_vcl.findChild(m_vcl.findObject("Form/Image/Title/Caption/sysButtons"), "CaptionRight");
    int capRightW = m_vcl.getInt(capRightObj, "Width", 5);
    int rightEdge = sysX + sysW - capRightW;

    // VCL TFormStyleHook.PaintNC draws buttons conditionally:
    // Close is always drawn (rightmost). Then Max (if biMaximize).
    // Then Min (if biMinimize). Each button uses GetElementContentRect
    // which positions them contiguously from right to left.
    // Inno Setup has biMinimize but NOT biMaximize, so: Close, Min.
    int closeX = rightEdge - m_wbW;
    int minX   = closeX - m_wbW;  // right next to Close

    m_closeRect = {closeX, capY, m_wbW, m_wbH};
    m_minRect   = {minX, capY, m_wbW, m_wbH};

    // For CaptionTitle text centering: the TextMarginRight=90 is a fixed
    // theme property that reserves space regardless of which buttons are shown.
    // Use all 5 button slots for the CaptionTitle width calculation.
    int helpX = rightEdge - 5 * m_wbW; // virtual position of leftmost button slot

    auto capTitle = m_vcl.findObject("Form/Image/Title/Caption/sysButtons/CaptionTitle");
    m_titleTextML = m_vcl.getInt(capTitle, "TextMarginLeft", 0);
    m_titleTextMR = m_vcl.getInt(capTitle, "TextMarginRight", 90);
    m_titleTextAlign = m_vcl.getString(capTitle, "TextAlign", "taCenter");

    // Title text rect: for taLeft, text starts after icon; for taCenter, use CaptionTitle area
    auto sysMenuObj = m_vcl.findObject("Form/Image/Title/Caption/btnSysMenu");
    int iconRight = m_vcl.getInt(sysMenuObj, "Left", 4) + m_vcl.getInt(sysMenuObj, "Width", 21) + 2;
    int capH = m_vcl.getInt(captionObj, "Height", 28);

    if (m_titleTextAlign == "taCenter") {
        // Center within CaptionTitle area (between sysX and helpX, with margins)
        int ctLeft = sysX;
        int ctWidth = helpX - sysX;
        m_titleTextRect = {ctLeft + m_titleTextML, capY,
                           ctWidth - m_titleTextML - m_titleTextMR, capH};
    } else {
        // taLeft: text starts after icon, extends to close button area
        m_titleTextRect = {iconRight, capY,
                           closeX - iconRight, capH};
    }

    // Logo (selected by logoNum, e.g. "5" → Logo5.bmp)
    m_logoTex = IMG_LoadTexture(m_renderer, (assetsDir + "/Logo" + logoNum + ".bmp").c_str());

    // Icon - load pre-extracted PNG (correct ICO entry)
    m_iconTex = IMG_LoadTexture(m_renderer, (assetsDir + "/Icon" + iconNum + ".png").c_str());
    if (!m_iconTex)
        m_iconTex = IMG_LoadTexture(m_renderer, (assetsDir + "/Icon" + iconNum + ".ico").c_str());

    // Music control textures - BMPs with black replaced by transparent
    // Port of setup.iss: ReplaceColor := clBlack; ReplaceWithColor := WizardForm.Color
    // Port of setup.iss: ReplaceColor := clBlack; ReplaceWithColor := WizardForm.Color
    // All music control BMPs get black pixels replaced with window background color
    auto wc = m_vcl.sysColor("clBtnFace");
    auto loadBmpReplaceBlack = [&](const std::string &path) -> SDL_Texture* {
        SDL_Surface *surf = SDL_LoadBMP(path.c_str());
        if (!surf) return nullptr;
        Uint32 blackPx = SDL_MapRGB(surf->format, 0, 0, 0);
        Uint32 replacePx = SDL_MapRGB(surf->format, wc.r, wc.g, wc.b);
        SDL_LockSurface(surf);
        Uint8 *pixels = (Uint8*)surf->pixels;
        for (int y = 0; y < surf->h; y++) {
            for (int x = 0; x < surf->w; x++) {
                Uint32 *px = (Uint32*)(pixels + y * surf->pitch + x * surf->format->BytesPerPixel);
                if (*px == blackPx) *px = replacePx;
            }
        }
        SDL_UnlockSurface(surf);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(m_renderer, surf);
        SDL_FreeSurface(surf);
        return tex;
    };
    m_playTex = loadBmpReplaceBlack(assetsDir + "/Play1.bmp");
    m_pauseMusicTex = loadBmpReplaceBlack(assetsDir + "/Pause1.bmp");
    m_trackBgTex = loadBmpReplaceBlack(assetsDir + "/TrackBkg.bmp");
    m_trackBtnTex = loadBmpReplaceBlack(assetsDir + "/TrackBtn1.bmp");

    // Colors from theme
    m_clrBorder = m_vcl.color("ktcBorder");
    m_clrEdit = m_vcl.color("ktcEdit");
    m_clrLabel = {0xe1, 0xe0, 0xe6, 255}; // setup.iss hardcodes $E6E0E1
    m_clrTitleText = m_vcl.fontColor("ktfCaptionTextNormal");
    m_clrEditText = m_vcl.fontColor("ktfEditBoxTextNormal");
    m_clrBtnTextN = m_vcl.fontColor("ktfButtonTextNormal");
    m_clrBtnTextH = m_vcl.fontColor("ktfButtonTextHot");
    m_clrBtnTextP = m_vcl.fontColor("ktfButtonTextPressed");
    m_clrBtnTextD = m_vcl.fontColor("ktfButtonTextDisabled");

    // Init text fields
    m_dirEdit.name = "DirEdit";
    m_dirEdit.text = "/home/" + std::string(getenv("USER") ? getenv("USER") : "user") + "/Games/Example Game";
    m_groupEdit.name = "GroupEdit";
    m_groupEdit.text = "Example Game";

    // Drives
    m_drives.push_back("/");
    // TODO: enumerate mount points

    // Init buttons from layout
    m_btnLeft.rect = lr("btnLeftButton");
    m_btnLeft.text = "Exit";
    m_btnRight.rect = lr("btnRightButton");
    m_btnRight.text = "Install";
    m_btnPause.rect = m_btnRight.rect;
    m_btnPause.text = "Pause";
    m_btnPause.visible = false;
    m_btnDirBrowse.rect = lr("btnDirBrowse");
    m_btnDirBrowse.text = "Browse...";
    m_btnGroupBrowse.rect = lr("btnGroupBrowse");
    m_btnGroupBrowse.text = "Browse...";

    // Log
    m_logLines.push_back("Waiting for Input...");

    // Audio
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
    std::string musicPath = assetsDir + "/Music" + musicNum + ".ogg";
    m_music = Mix_LoadMUS(musicPath.c_str());
    if (m_music) Mix_PlayMusic(m_music, -1);

    m_lastTick = SDL_GetTicks();
    return true;
}

// =============================================================================
// Layout helper
// =============================================================================

SDL_Rect InstallerWindow::lr(const std::string &name) const {
    if (!m_layout.contains(name)) return {0,0,0,0};
    auto o = m_layout[name];
    // Layout positions are relative to client area; offset by border and title bar
    return {m_borderL + o.value("x",0), m_titleH + o.value("y",0), o.value("w",0), o.value("h",0)};
}

// =============================================================================
// Main loop
// =============================================================================

void InstallerWindow::run() {
    while (m_running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) m_running = false;
            else handleEvent(e);
        }

        // Progress timer
        if (m_state == Installing && !m_paused) {
            Uint32 now = SDL_GetTicks();
            if (now - m_lastTick >= 50) {
                m_lastTick = now;
                m_progressValue += 5;
                if (m_progressValue % 100 == 0) {
                    m_logLines.push_back("Extracting file " + std::to_string(m_progressValue/10) + " of 100...");
                }
                if (m_progressValue > 1000) setState(Finished);
            }
        }

        render();
        SDL_RenderPresent(m_renderer);
        SDL_Delay(1); // don't burn CPU
    }
}

// =============================================================================
// Render - ALL drawing happens here
// =============================================================================

void InstallerWindow::render() {
    // Clear to black (fallback, client bg will cover this)
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);

    renderFrame();
    renderWidgets();
}

void InstallerWindow::renderFrame() {
    int w = m_borderL + CLIENT_W + m_borderR;
    int h = 618;

    // Title bar (9-patch from theme)
    SDL_Rect titleDst = {0, 0, w, m_titleH};
    m_vcl.draw9PatchTex(m_titlebarTex, m_titleML, m_titleMT, m_titleMR, m_titleMB, titleDst);

    // Bottom border first
    SDL_Rect bbDst = {0, h - m_borderB, w, m_borderB};
    m_vcl.draw9PatchTex(m_borderBTex, m_bbML, m_bbMT, m_bbMR, m_bbMB, bbDst);

    // Left border - extends full height, overlaps bottom border
    int bTop = m_titleH;
    int bH = h - m_titleH;
    SDL_Rect lbDst = {0, bTop, m_borderL, bH};
    m_vcl.draw9PatchTex(m_borderLTex, m_blML, m_blMT, m_blMR, m_blMB, lbDst);

    // Right border - extends full height, overlaps bottom border
    SDL_Rect rbDst = {w - m_borderR, bTop, m_borderR, bH};
    m_vcl.draw9PatchTex(m_borderRTex, m_brML, m_brMT, m_brMR, m_brMB, rbDst);

    // Client area - fill with window color (clBtnFace), matching VCL form rendering
    int cx = m_borderL, cy = m_titleH;
    int cw = w - m_borderL - m_borderR, ch = h - m_titleH - m_borderB;
    auto winColor = m_vcl.sysColor("clBtnFace");
    SDL_SetRenderDrawColor(m_renderer, winColor.r, winColor.g, winColor.b, 255);
    SDL_Rect clientDst = {cx, cy, cw, ch};
    SDL_RenderFillRect(m_renderer, &clientDst);

    // Icon in title bar
    if (m_iconTex) {
        // Position from btnSysMenu: Left=7, Top=2, Width=21, Height=20
        // Icon centered within that rect
        auto sysMenuObj = m_vcl.findObject("Form/Image/Title/Caption/btnSysMenu");
        int smL = 4;
        int smT = m_vcl.getInt(sysMenuObj, "Top", 2);
        int smW = m_vcl.getInt(sysMenuObj, "Width", 21);
        int smH = m_vcl.getInt(sysMenuObj, "Height", 20);
        SDL_Rect id = {smL + (smW - 16) / 2, smT + (smH - 16) / 2, 16, 16};
        SDL_RenderCopy(m_renderer, m_iconTex, nullptr, &id);
    }

    // Title text - drawn in the computed CaptionTitle rect (from VCL Aligning)
    m_vcl.drawText("Example Game", m_vcl.getTitleFont(), m_clrTitleText,
                   m_titleTextRect, m_titleTextAlign);

    // Window buttons
    auto drawWndBtn = [&](const SDL_Rect &r, SDL_Texture *n, SDL_Texture *h_t, bool hover) {
        SDL_Texture *t = hover ? h_t : n;
        if (!t) return;
        int tw, th; SDL_QueryTexture(t, nullptr, nullptr, &tw, &th);
        SDL_Rect d = {r.x + (r.w - tw)/2, r.y + (r.h - th)/2, tw, th};
        SDL_RenderCopy(m_renderer, t, nullptr, &d);
    };
    // Only Close and Min are drawn (no WS_MAXIMIZEBOX on the installer window)
    // Max/Res/Help slots exist in the layout but are not rendered
    drawWndBtn(m_closeRect, m_wbCloseN, m_wbCloseH, m_closeHover);
    drawWndBtn(m_minRect, m_wbMinN, m_wbMinH, m_minHover);
}

void InstallerWindow::renderWidgets() {
    // Logo
    if (m_logoTex) {
        SDL_Rect d = lr("WizardBitmapImage");
        SDL_RenderCopy(m_renderer, m_logoTex, nullptr, &d);
    }

    // Bevels - port of DrawStyleEdge from Vcl.Themes.pas
    // Uses cl3DLight/cl3DDkShadow for outer, clBtnHighlight/clBtnShadow for inner
    auto c3DLight   = m_vcl.sysColor("cl3DLight");
    auto c3DDkShadow = m_vcl.sysColor("cl3DDkShadow");
    auto cBtnHL     = m_vcl.sysColor("clBtnHighlight");
    auto cBtnShadow = m_vcl.sysColor("clBtnShadow");

    // DrawStyleEdge inner helper: draws one frame with highlight on specified sides
    auto drawEdgeRect = [&](SDL_Rect r, SDL_Color hiColor, SDL_Color shColor,
                            bool topLeftHi, bool bottomRightHi) {
        // Top + Left
        auto &tlc = topLeftHi ? hiColor : shColor;
        SDL_SetRenderDrawColor(m_renderer, tlc.r, tlc.g, tlc.b, 255);
        SDL_RenderDrawLine(m_renderer, r.x, r.y + r.h - 1, r.x, r.y);           // left
        SDL_RenderDrawLine(m_renderer, r.x, r.y, r.x + r.w - 1, r.y);           // top
        // Bottom + Right
        auto &brc = bottomRightHi ? hiColor : shColor;
        SDL_SetRenderDrawColor(m_renderer, brc.r, brc.g, brc.b, 255);
        SDL_RenderDrawLine(m_renderer, r.x + r.w - 1, r.y, r.x + r.w - 1, r.y + r.h - 1); // right
        SDL_RenderDrawLine(m_renderer, r.x + r.w - 1, r.y + r.h - 1, r.x, r.y + r.h - 1); // bottom
    };

    // bsRaised: highlight on top/left, shadow on bottom/right (single 1px edge)
    auto drawBevelRaised = [&](const std::string &name) {
        SDL_Rect r = lr(name);
        drawEdgeRect(r, cBtnHL, cBtnShadow, true, false);
    };

    // bsLowered: shadow on top/left, highlight on bottom/right (single 1px edge)
    auto drawBevelLowered = [&](const std::string &name) {
        SDL_Rect r = lr(name);
        drawEdgeRect(r, cBtnHL, cBtnShadow, false, true);
    };

    // setup.iss: bvlDirectories is bsRaised, all others are bsLowered
    drawBevelRaised("bvlDirectories");
    drawBevelLowered("bvlDirInstall");
    drawBevelLowered("bvlIconGroup");
    drawBevelLowered("bvlOptions");
    drawBevelLowered("bvlInstallOptions");
    drawBevelLowered("bvlButtons");
    drawBevelLowered("bvlLeftButton");
    drawBevelLowered("bvlRightButton");
    drawBevelLowered("bvlProgressForm");
    drawBevelLowered("bvlProgressGauge");

    renderEdits();
    renderLabels();
    renderCheckboxes();
    renderButtons();
    renderProgress();
    renderAudio();
    renderResult();
}

void InstallerWindow::renderEdits() {
    // Dir edit: 9-patch frame bitmap, then fill edit client area with ktcEdit
    // Frame bitmap margins (4px) for 9-patch slicing
    // Frame object margins (2px) define the edit client inset
    SDL_Rect dirR = lr("DirEdit");
    m_vcl.draw9PatchTex(m_editTex, m_edML, m_edMT, m_edMR, m_edMB, dirR);
    SDL_Rect inner = {dirR.x+m_edFML, dirR.y+m_edFMT, dirR.w-m_edFML-m_edFMR, dirR.h-m_edFMT-m_edFMB};
    SDL_SetRenderDrawColor(m_renderer, m_clrEdit.r, m_clrEdit.g, m_clrEdit.b, 255);
    SDL_RenderFillRect(m_renderer, &inner);
    SDL_Rect textR = {inner.x+2, inner.y, inner.w-4, inner.h};
    m_vcl.drawText(m_dirEdit.text, m_vcl.getFont(""), m_clrEditText, textR, "taLeft");
    // Cursor
    if (m_dirEdit.focused) {
        int cx = inner.x + 2;
        // Simple: draw cursor at end
        if (m_vcl.getFont("")) {
            int tw, th;
            TTF_SizeUTF8(m_vcl.getFont(""), m_dirEdit.text.c_str(), &tw, &th);
            cx += std::min(tw, inner.w - 4);
        }
        SDL_SetRenderDrawColor(m_renderer, m_clrEditText.r, m_clrEditText.g, m_clrEditText.b, 255);
        SDL_RenderDrawLine(m_renderer, cx, inner.y+2, cx, inner.y+inner.h-2);
    }

    // Group edit
    SDL_Rect grR = lr("GroupEdit");
    m_vcl.draw9PatchTex(m_editTex, m_edML, m_edMT, m_edMR, m_edMB, grR);
    SDL_Rect grInner = {grR.x+m_edFML, grR.y+m_edFMT, grR.w-m_edFML-m_edFMR, grR.h-m_edFMT-m_edFMB};
    SDL_SetRenderDrawColor(m_renderer, m_clrEdit.r, m_clrEdit.g, m_clrEdit.b, 255);
    SDL_RenderFillRect(m_renderer, &grInner);
    SDL_Rect grTextR = {grInner.x+2, grInner.y, grInner.w-4, grInner.h};
    m_vcl.drawText(m_groupEdit.text, m_vcl.getFont(""), m_clrEditText, grTextR, "taLeft");

    // Drive combo - port of TSysComboBoxStyleHook.PaintBorder:
    // 1. DrawElement(tcBorderNormal) = edit frame at full rect
    // 2. DrawElement(tcDropDownButtonNormal) = button at ButtonRect
    // 3. ExcludeClipRect for text area (InflateRect(-3,-3), Right = ButtonRect.Left - 2)
    //    We simulate this by filling the text area with clBtnFace AFTER drawing frame
    SDL_Rect cbR = lr("cbxDrive");

    // Button width matching the original installer (16px)
    int comboBtnW = 16;
    SDL_Rect btnR = {cbR.x + cbR.w - m_edFMR - comboBtnW, cbR.y + m_edFMT,
                     comboBtnW, cbR.h - m_edFMT - m_edFMB};

    // 1. Draw edit frame border at full combo rect
    m_vcl.draw9PatchTex(m_comboTex, m_cbML, m_cbMT, m_cbMR, m_cbMB, cbR);

    // 2. Fill text area with clWindow (the combo control's own background)
    auto cbBg = m_vcl.sysColor("clWindow");
    SDL_Rect cbFill = {cbR.x + 3, cbR.y + 3,
                       btnR.x - 1 - (cbR.x + 3),
                       cbR.h - 6};
    SDL_SetRenderDrawColor(m_renderer, cbBg.r, cbBg.g, cbBg.b, 255);
    SDL_RenderFillRect(m_renderer, &cbFill);

    // Text in the fill area
    std::string driveText = m_drives.empty() ? "/" : m_drives[m_activeDrive];
    SDL_Rect cbTextR = {cbFill.x + 2, cbFill.y, cbFill.w - 4, cbFill.h};
    m_vcl.drawText(driveText, m_vcl.getFont(""), m_clrEditText, cbTextR, "taLeft");

    // 3. Draw dropdown button
    if (m_comboBtnN) {
        m_vcl.draw9PatchTex(m_comboBtnN, m_cbBtnML, m_cbBtnMT, m_cbBtnMR, m_cbBtnMB, btnR);
        // Arrow glyph centered on the button
        if (m_comboArrowTex) {
            int aw, ah; SDL_QueryTexture(m_comboArrowTex, nullptr, nullptr, &aw, &ah);
            SDL_Rect ad = {btnR.x + (btnR.w - aw)/2, btnR.y + (btnR.h - ah)/2, aw, ah};
            SDL_RenderCopy(m_renderer, m_comboArrowTex, nullptr, &ad);
        }
    } else if (m_comboArrowTex) {
        int aw, ah; SDL_QueryTexture(m_comboArrowTex, nullptr, nullptr, &aw, &ah);
        SDL_Rect ad = {cbR.x+cbR.w-m_cbMR-aw-2, cbR.y+(cbR.h-ah)/2, aw, ah};
        SDL_RenderCopy(m_renderer, m_comboArrowTex, nullptr, &ad);
    }
}

void InstallerWindow::renderLabels() {
    TTF_Font *f = m_vcl.getLabelFont();
    m_vcl.drawText("Install directory", f, m_clrLabel, lr("lblDirInstall"), "taLeft");
    m_vcl.drawText("Create desktop shortcut", f, m_clrLabel, lr("lblDesktopIcon"), "taLeft");
    m_vcl.drawText("Directory at Start Menu", f, m_clrLabel, lr("lblGroupDir"), "taLeft");
    m_vcl.drawText("Create a Start Menu folder", f, m_clrLabel, lr("lblCreateGroup"), "taLeft");

    // Disk size label - AutoSize in setup.iss, so use parent bevel width
    char buf[128];
    snprintf(buf, sizeof(buf), "At least %.2f GB of free space required", 4616.0 / 1024.0);
    SDL_Rect diskR = lr("lblDiskSizeNeeded");
    SDL_Rect optR = lr("bvlInstallOptions");
    diskR.w = optR.x + optR.w - diskR.x - 10; // expand to fill parent
    // Red if insufficient space (matching setup.iss DirEditOnChange: $1B1BE7 = #e71b1b)
    // For now assume always enough space on Linux
    m_vcl.drawText(buf, f, m_clrLabel, diskR, "taLeft");

    m_vcl.drawText("Do not create uninstaller and do not write any specific system info",
                   f, m_clrLabel, lr("lblNoUninstaller"), "taLeft");
}

void InstallerWindow::renderCheckboxes() {
    bool enabled = (m_state == SelectDir);
    auto drawChk = [&](const std::string &name, bool checked) {
        SDL_Rect r = lr(name);
        SDL_Texture *t = !enabled ? (checked ? m_chkCD : m_chkUD)
                       : checked  ? m_chkCN : m_chkUN;
        if (t) SDL_RenderCopy(m_renderer, t, nullptr, &r);
    };
    drawChk("chbDesktopIcon", m_chkDesktop);
    drawChk("chbCreateGroup", m_chkGroup);
    drawChk("chbNoUninstaller", m_chkNoUninst);
}

void InstallerWindow::renderButtons() {
    auto drawBtn = [&](const Button &b) {
        if (!b.visible) return;
        SDL_Texture *t = !b.enabled ? m_btnDTex : b.pressed ? m_btnPTex : b.hover ? m_btnHTex : m_btnNTex;
        SDL_Color tc = !b.enabled ? m_clrBtnTextD : b.pressed ? m_clrBtnTextP : b.hover ? m_clrBtnTextH : m_clrBtnTextN;
        m_vcl.draw9PatchTex(t, m_btnML, m_btnMT, m_btnMR, m_btnMB, b.rect);
        m_vcl.drawText(b.text, m_vcl.getButtonFont(), tc, b.rect, "taCenter");
    };
    drawBtn(m_btnLeft);
    drawBtn(m_btnRight);
    drawBtn(m_btnPause);
    drawBtn(m_btnDirBrowse);
    drawBtn(m_btnGroupBrowse);
}

void InstallerWindow::renderProgress() {
    SDL_Rect r = lr("ProgressGauge");
    m_vcl.draw9PatchTex(m_progFrameTex, m_pfML, m_pfMT, m_pfMR, m_pfMB, r);
    if (m_progressValue > 0) {
        int barW = (r.w - m_pfML - m_pfMR) * m_progressValue / 1000;
        if (barW > 0) {
            SDL_Rect barR = {r.x + m_pfML, r.y + m_pfMT, barW, r.h - m_pfMT - m_pfMB};
            m_vcl.draw9PatchTex(m_progBarTex, m_pbML, m_pbMT, m_pbMR, m_pbMB, barR);
        }
    }

    // Log area background
    SDL_Rect logR = lr("memProgressLog");
    SDL_SetRenderDrawColor(m_renderer, m_clrEdit.r, m_clrEdit.g, m_clrEdit.b, 255);
    SDL_RenderFillRect(m_renderer, &logR);
    SDL_SetRenderDrawColor(m_renderer, m_clrBorder.r, m_clrBorder.g, m_clrBorder.b, 255);
    SDL_RenderDrawRect(m_renderer, &logR);

    // Log text
    TTF_Font *f = m_vcl.getFont("");
    if (f) {
        int lineH = TTF_FontLineSkip(f);
        int maxLines = (logR.h - 4) / lineH;
        int startLine = std::max(0, (int)m_logLines.size() - maxLines);
        for (int i = startLine; i < (int)m_logLines.size(); i++) {
            int y = logR.y + 2 + (i - startLine) * lineH;
            SDL_Rect tr = {logR.x + 4, y, logR.w - 8, lineH};
            m_vcl.drawText(m_logLines[i], f, m_clrEditText, tr, "taLeft");
        }
    }
}

void InstallerWindow::renderAudio() {
    // Play/Pause buttons
    SDL_Rect playR = lr("bmpPlayButton");
    SDL_Rect pauseR = lr("bmpPauseButton");
    if (m_playTex) SDL_RenderCopy(m_renderer, m_playTex, nullptr, &playR);
    if (m_pauseMusicTex) SDL_RenderCopy(m_renderer, m_pauseMusicTex, nullptr, &pauseR);

    // Volume track + thumb (thumb has black border replaced with window color)
    SDL_Rect trackR = lr("bmpTrackBar");
    if (m_trackBgTex) SDL_RenderCopy(m_renderer, m_trackBgTex, nullptr, &trackR);
    if (m_trackBtnTex) {
        int tw, th;
        SDL_QueryTexture(m_trackBtnTex, nullptr, nullptr, &tw, &th);
        int slideRange = trackR.w - tw;
        int thumbX = trackR.x + (int)(m_volume * slideRange);
        int thumbY = trackR.y + (trackR.h - th) / 2;
        SDL_Rect tbR = {thumbX, thumbY, tw, th};
        SDL_RenderCopy(m_renderer, m_trackBtnTex, nullptr, &tbR);
    }
}

void InstallerWindow::renderResult() {
    if (!m_showResult) return;
    SDL_Rect r = lr("lblInstallResult");
    SDL_Color c = m_showResult ? SDL_Color{0x00, 0xdd, 0x34, 255} : SDL_Color{0xe7, 0x1b, 0x1b, 255};
    m_vcl.drawText("Successfully Installed", m_vcl.getTitleFont(), c, r, "taCenter");
}

// =============================================================================
// Events
// =============================================================================

static bool inRect(int x, int y, const SDL_Rect &r) {
    return x >= r.x && x < r.x+r.w && y >= r.y && y < r.y+r.h;
}

void InstallerWindow::handleEvent(const SDL_Event &e) {
    switch (e.type) {
    case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_LEFT)
            handleMouseDown(e.button.x, e.button.y);
        break;
    case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_LEFT)
            handleMouseUp(e.button.x, e.button.y);
        break;
    case SDL_MOUSEMOTION:
        handleMouseMove(e.motion.x, e.motion.y);
        break;
    case SDL_TEXTINPUT:
        handleTextInput(e.text.text);
        break;
    case SDL_KEYDOWN:
        handleKeyDown(e.key.keysym.sym);
        break;
    }
}

void InstallerWindow::handleMouseDown(int x, int y) {
    // Window buttons
    if (inRect(x, y, m_closeRect)) { m_running = false; return; }
    if (inRect(x, y, m_minRect)) { SDL_MinimizeWindow(m_window); return; }

    // Focus text fields
    m_dirEdit.focused = inRect(x, y, lr("DirEdit"));
    m_groupEdit.focused = inRect(x, y, lr("GroupEdit"));

    // Checkboxes + labels
    if (m_state == SelectDir) {
        if (inRect(x, y, lr("chbDesktopIcon")) || inRect(x, y, lr("lblDesktopIcon")))
            m_chkDesktop = !m_chkDesktop;
        if (inRect(x, y, lr("chbCreateGroup")) || inRect(x, y, lr("lblCreateGroup"))) {
            m_chkGroup = !m_chkGroup;
            m_groupEdit.enabled = m_chkGroup;
        }
        if (inRect(x, y, lr("chbNoUninstaller")) || inRect(x, y, lr("lblNoUninstaller")))
            m_chkNoUninst = !m_chkNoUninst;
    }

    // Buttons
    auto pressBtn = [&](Button &b) {
        if (b.visible && b.enabled && inRect(x, y, b.rect)) b.pressed = true;
    };
    pressBtn(m_btnLeft);
    pressBtn(m_btnRight);
    pressBtn(m_btnPause);
    pressBtn(m_btnDirBrowse);
    pressBtn(m_btnGroupBrowse);

    // Music play/pause
    if (inRect(x, y, lr("bmpPlayButton"))) {
        if (m_music && !Mix_PlayingMusic())
            Mix_PlayMusic(m_music, -1);
        else if (Mix_PausedMusic())
            Mix_ResumeMusic();
    }
    if (inRect(x, y, lr("bmpPauseButton"))) {
        if (Mix_PlayingMusic() && !Mix_PausedMusic())
            Mix_PauseMusic();
    }

    // Volume - click on track/thumb area starts drag
    SDL_Rect trackR = lr("bmpTrackBar");
    SDL_Rect volHitR = {trackR.x - 4, trackR.y - 6, trackR.w + 8, trackR.h + 12};
    if (inRect(x, y, volHitR)) {
        m_draggingVolume = true;
        m_volume = float(x - trackR.x) / trackR.w;
        m_volume = std::max(0.0f, std::min(1.0f, m_volume));
        Mix_VolumeMusic(int(m_volume * MIX_MAX_VOLUME));
    }

    // Drag
    if (y < m_titleH + 65 && !inRect(x, y, m_closeRect) && !inRect(x, y, m_minRect)) {
        m_dragging = true;
        m_dragX = x; m_dragY = y;
    }
}

void InstallerWindow::handleMouseUp(int x, int y) {
    m_dragging = false;
    m_draggingVolume = false;
    if (m_btnLeft.pressed) {
        m_btnLeft.pressed = false;
        if (inRect(x, y, m_btnLeft.rect)) {
            if (m_state == Installing) {
                setState(SelectDir);
                m_logLines.push_back("Cancelled.");
            } else if (m_state == Finished) {
                // "Run" button - would launch the game
            } else {
                m_running = false;
            }
        }
    }
    if (m_btnRight.pressed) {
        m_btnRight.pressed = false;
        if (inRect(x, y, m_btnRight.rect)) {
            if (m_state == SelectDir) setState(Installing);
            else if (m_state == Finished) m_running = false;
        }
    }
    if (m_btnPause.pressed) {
        m_btnPause.pressed = false;
        if (inRect(x, y, m_btnPause.rect)) {
            m_paused = !m_paused;
            m_btnPause.text = m_paused ? "Resume" : "Pause";
        }
    }
}

void InstallerWindow::handleMouseMove(int x, int y) {
    m_closeHover = inRect(x, y, m_closeRect);
    m_minHover = inRect(x, y, m_minRect);

    auto updateHover = [&](Button &b) {
        b.hover = b.visible && b.enabled && inRect(x, y, b.rect);
    };
    updateHover(m_btnLeft);
    updateHover(m_btnRight);
    updateHover(m_btnPause);
    updateHover(m_btnDirBrowse);
    updateHover(m_btnGroupBrowse);

    // Volume drag
    if (m_draggingVolume) {
        SDL_Rect trackR = lr("bmpTrackBar");
        m_volume = float(x - trackR.x) / trackR.w;
        m_volume = std::max(0.0f, std::min(1.0f, m_volume));
        Mix_VolumeMusic(int(m_volume * MIX_MAX_VOLUME));
    }

    if (m_dragging && !m_draggingVolume) {
        int wx, wy;
        SDL_GetWindowPosition(m_window, &wx, &wy);
        SDL_SetWindowPosition(m_window, wx + x - m_dragX, wy + y - m_dragY);
    }
}

void InstallerWindow::handleTextInput(const char *text) {
    if (m_dirEdit.focused && m_dirEdit.enabled)
        m_dirEdit.text += text;
    else if (m_groupEdit.focused && m_groupEdit.enabled)
        m_groupEdit.text += text;
}

void InstallerWindow::handleKeyDown(SDL_Keycode key) {
    TextField *active = nullptr;
    if (m_dirEdit.focused) active = &m_dirEdit;
    else if (m_groupEdit.focused) active = &m_groupEdit;
    if (!active || !active->enabled) return;

    if (key == SDLK_BACKSPACE && !active->text.empty())
        active->text.pop_back();
}

// =============================================================================
// State machine
// =============================================================================

void InstallerWindow::setState(State s) {
    m_state = s;
    if (s == SelectDir) {
        m_dirEdit.enabled = true;
        m_groupEdit.enabled = m_chkGroup;
        m_btnLeft = {lr("btnLeftButton"), "Exit", false, false, true, true};
        m_btnRight = {lr("btnRightButton"), "Install", false, false, true, true};
        m_btnPause.visible = false;
        m_showResult = false;
    } else if (s == Installing) {
        m_dirEdit.enabled = false;
        m_groupEdit.enabled = false;
        m_dirEdit.focused = false;
        m_groupEdit.focused = false;
        m_btnLeft = {lr("btnLeftButton"), "Cancel", false, false, true, true};
        m_btnRight.visible = false;
        m_btnPause = {lr("btnRightButton"), "Pause", false, false, true, true};
        m_progressValue = 0;
        m_paused = false;
        m_logLines.clear();
        m_logLines.push_back("Extracting files...");
        m_lastTick = SDL_GetTicks();
    } else if (s == Finished) {
        m_btnPause.visible = false;
        // Left cell: "Run" button (btnRun in setup.iss, same position as btnLeftButton)
        m_btnLeft = {lr("btnLeftButton"), "Run", false, false, true, true};
        // Right cell: "Finish" button
        m_btnRight = {lr("btnRightButton"), "Finish", false, false, true, true};
        m_showResult = true;
        m_logLines.push_back("Done!");
    }
}

// =============================================================================
// Cleanup
// =============================================================================

void InstallerWindow::cleanup() {
    if (m_music) { Mix_FreeMusic(m_music); m_music = nullptr; }
    Mix_CloseAudio();
    if (m_logoTex) SDL_DestroyTexture(m_logoTex);
    if (m_iconTex) SDL_DestroyTexture(m_iconTex);
    if (m_playTex) SDL_DestroyTexture(m_playTex);
    if (m_pauseMusicTex) SDL_DestroyTexture(m_pauseMusicTex);
    if (m_trackBgTex) SDL_DestroyTexture(m_trackBgTex);
    if (m_trackBtnTex) SDL_DestroyTexture(m_trackBtnTex);
    // VclRenderer destructor handles its textures
    if (m_renderer) SDL_DestroyRenderer(m_renderer);
    if (m_window) SDL_DestroyWindow(m_window);
}
