#pragma once

#include "VclRenderer.h"
#include <SDL.h>
#include <SDL_mixer.h>
#include <string>
#include <vector>

/**
 * InstallerWindow - pure SDL2 rendering, direct port of VCL style logic.
 * No widget system. All drawing via SDL_RenderCopy.
 * Layout from layout.json. Theme from theme.json. Fonts from fonts/.
 */
class InstallerWindow {
public:
    bool init(const std::string &assetsDir, const std::string &themeDir,
              const std::string &layoutPath, const std::string &fontsDir,
              const std::string &logoNum = "5", const std::string &iconNum = "1",
              const std::string &musicNum = "1");
    void run(); // main loop
    void cleanup();

private:
    enum State { SelectDir, Installing, Finished };

    // --- Rendering ---
    void render();
    void renderFrame();       // title bar, borders, client bg
    void renderWidgets();     // all controls
    void renderButtons();
    void renderCheckboxes();
    void renderLabels();
    void renderEdits();
    void renderProgress();
    void renderAudio();
    void renderResult();

    // --- Events ---
    void handleEvent(const SDL_Event &e);
    void handleMouseDown(int x, int y);
    void handleMouseUp(int x, int y);
    void handleMouseMove(int x, int y);
    void handleTextInput(const char *text);
    void handleKeyDown(SDL_Keycode key);

    // --- State ---
    void setState(State s);

    // --- Layout helper ---
    SDL_Rect lr(const std::string &name) const; // layout rect, Y offset by titleH

    // --- Data ---
    SDL_Window *m_window = nullptr;
    SDL_Renderer *m_renderer = nullptr;
    VclRenderer m_vcl;
    json m_layout;
    std::string m_assetsDir;
    bool m_running = true;
    State m_state = SelectDir;

    // Frame dimensions (from theme)
    int m_titleH = 0, m_borderL = 0, m_borderR = 0, m_borderB = 0;

    // Text fields
    struct TextField {
        std::string text;
        std::string name; // layout key
        int cursor = 0;
        bool focused = false;
        bool enabled = true;
    };
    TextField m_dirEdit, m_groupEdit;
    int m_activeDrive = 0;
    std::vector<std::string> m_drives;

    // Checkboxes
    bool m_chkDesktop = true, m_chkGroup = true, m_chkNoUninst = false;

    // Buttons
    struct Button {
        SDL_Rect rect;
        std::string text;
        bool hover = false, pressed = false, enabled = true, visible = true;
    };
    Button m_btnLeft, m_btnRight, m_btnPause;
    Button m_btnDirBrowse, m_btnGroupBrowse;

    // Window buttons
    SDL_Rect m_closeRect, m_minRect;
    bool m_closeHover = false, m_minHover = false;

    // Progress
    int m_progressValue = 0;
    bool m_paused = false;
    Uint32 m_lastTick = 0;
    bool m_showResult = false;

    // Log
    std::vector<std::string> m_logLines;

    // Drag
    bool m_dragging = false;
    int m_dragX = 0, m_dragY = 0;

    // Audio
    Mix_Music *m_music = nullptr;

    // Pre-loaded textures (from theme assets)
    SDL_Texture *m_logoTex = nullptr;
    SDL_Texture *m_btnNTex=nullptr, *m_btnHTex=nullptr, *m_btnPTex=nullptr, *m_btnDTex=nullptr;
    int m_btnML=0, m_btnMT=0, m_btnMR=0, m_btnMB=0;
    SDL_Texture *m_editTex = nullptr;
    int m_edML=0, m_edMT=0, m_edMR=0, m_edMB=0; // bitmap 9-patch margins
    int m_edFML=0, m_edFMT=0, m_edFMR=0, m_edFMB=0; // Frame margins (edit client inset)
    SDL_Texture *m_comboTex = nullptr;
    int m_cbML=0, m_cbMT=0, m_cbMR=0, m_cbMB=0;
    SDL_Texture *m_comboBtnN=nullptr, *m_comboBtnH=nullptr;
    int m_cbBtnML=0, m_cbBtnMT=0, m_cbBtnMR=0, m_cbBtnMB=0;
    int m_cbBtnW=17; // ComboBox/Button width from theme (default ~17px)
    SDL_Texture *m_chkUN=nullptr, *m_chkUH=nullptr, *m_chkUD=nullptr;
    SDL_Texture *m_chkCN=nullptr, *m_chkCH=nullptr, *m_chkCD=nullptr;
    SDL_Texture *m_progFrameTex=nullptr, *m_progBarTex=nullptr;
    int m_pfML=0,m_pfMT=0,m_pfMR=0,m_pfMB=0;
    int m_pbML=0,m_pbMT=0,m_pbMR=0,m_pbMB=0;
    SDL_Texture *m_wbCloseN=nullptr, *m_wbCloseH=nullptr;
    SDL_Texture *m_wbMaxN=nullptr, *m_wbMaxH=nullptr;
    SDL_Texture *m_wbResN=nullptr, *m_wbResH=nullptr;
    SDL_Texture *m_wbMinN=nullptr, *m_wbMinH=nullptr;
    SDL_Texture *m_wbHelpN=nullptr, *m_wbHelpH=nullptr;
    SDL_Rect m_maxRect={}, m_resRect={}, m_helpRect={};
    SDL_Texture *m_titlebarTex=nullptr;
    int m_titleML=0, m_titleMT=0, m_titleMR=0, m_titleMB=0;
    SDL_Texture *m_borderLTex=nullptr, *m_borderRTex=nullptr, *m_borderBTex=nullptr;
    int m_blML=0,m_blMT=0,m_blMR=0,m_blMB=0;
    int m_brML=0,m_brMT=0,m_brMR=0,m_brMB=0;
    int m_bbML=0,m_bbMT=0,m_bbMR=0,m_bbMB=0;
    SDL_Texture *m_clientBgTex = nullptr;
    std::string m_clientTile;
    SDL_Texture *m_comboArrowTex = nullptr;
    SDL_Texture *m_iconTex = nullptr;

    // Title text properties from theme
    int m_titleTextML=0, m_titleTextMR=0;
    std::string m_titleTextAlign;
    SDL_Rect m_titleTextRect = {0,0,0,0}; // computed CaptionTitle rect

    // Music control textures
    SDL_Texture *m_playTex=nullptr, *m_pauseMusicTex=nullptr;
    SDL_Texture *m_trackBgTex=nullptr, *m_trackBtnTex=nullptr;
    float m_volume = 0.58f;
    bool m_draggingVolume = false;

    // Colors from theme
    SDL_Color m_clrBorder, m_clrEdit, m_clrLabel, m_clrBtnTextN, m_clrBtnTextH,
              m_clrBtnTextP, m_clrBtnTextD, m_clrTitleText, m_clrEditText;

    int m_wbW=0, m_wbH=0, m_wbY=0;
};
