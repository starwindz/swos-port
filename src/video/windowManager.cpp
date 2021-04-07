#include "windowManager.h"
#include "util.h"

static SDL_Window *m_window;
static auto m_windowMode =
#ifdef __ANDROID__
    kModeFullScreen;
#else
    kModeWindow;
#endif

constexpr int kDefaultFullScreenWidth = 640;
constexpr int kDefaultFullScreenHeight = 480;

static int m_windowWidth = kWindowWidth;
static int m_windowHeight = kWindowHeight;
static int m_fieldWidth = kGameScreenWidth;
static int m_fieldHeight = kVgaHeight;

// full-screen dimensions
static int m_displayWidth = kDefaultFullScreenWidth;
static int m_displayHeight = kDefaultFullScreenHeight;
static bool m_windowResizable = true;

static bool m_flashMenuCursor = true;
#ifdef VIRTUAL_JOYPAD
static bool m_showTouchTrails = true;
static bool m_transparentButtons = true;
#endif

// video options
const char kVideoSection[] = "video";
const char kWindowMode[] = "windowMode";
const char kWindowWidthKey[] = "windowWidth";
const char kWindowHeightKey[] = "windowHeight";
const char kFieldWidthKey[] = "fieldWidth";
const char kFieldHeightKey[] = "fieldHeight";
const char kWindowResizable[] = "windowResizable";
const char kFullScreenWidth[] = "fullScreenWidth";
const char kFullScreenHeight[] = "fullScreenHeight";
const char kFlashMenuCursor[] = "flashMenuCursor";
#ifdef VIRTUAL_JOYPAD
const char kShowTouchTrails[] = "showTouchTrails";
const char kTransparentButtons[] = "virtualJoypadTransparentButtons";
#endif

static void setWindowMode(WindowMode newMode);

SDL_Window *createWindow()
{
    int flags = SDL_WINDOW_SHOWN;
    if (m_windowResizable)
        flags |= SDL_WINDOW_RESIZABLE;

    logInfo("Creating %dx%d window, flags: %d", m_windowWidth, m_windowHeight, flags);
    m_window = SDL_CreateWindow("SWOS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_windowWidth, m_windowHeight, flags);
    if (!m_window)
        sdlErrorExit("Could not create window");

    setWindowMode(m_windowMode);

    return m_window;
}

void destroyWindow()
{
    if (m_window)
        SDL_DestroyWindow(m_window);
}

SDL_Window *getWindow()
{
    return m_window;
}

std::pair<int, int> getWindowSize()
{
    assert(m_window);

   int width = -1, height = -1;

    if (m_windowMode == kModeWindow || m_windowMode == kModeBorderlessMaximized) {
        SDL_GetWindowSize(m_window, &width, &height);

        if (width > 0 && height > 0) {
            if (m_windowMode == kModeWindow) {
                m_windowWidth = width;
                m_windowHeight = height;
            }

            return { width, height };
        }
    }

    return { m_windowWidth, m_windowHeight };
}

static void clampWindowSize(int& width, int& height)
{
    assert(m_window);

    auto displayIndex = SDL_GetWindowDisplayIndex(m_window);
    if (displayIndex >= 0) {
        SDL_Rect rect;
        if (SDL_GetDisplayBounds(displayIndex, &rect) == 0) {
            width = std::min(width, rect.w);
            height = std::min(height, rect.h);
            logInfo("Clamping window size to %dx%d", width, height);
        }
    }
}

static void normalizeWindowSize(int& width, int& height)
{
    if (width <= 0 || height <= 0) {
        logInfo("Got invalid width/height (%dx%d), setting to default", width, height);
        width = kWindowWidth;
        height = kWindowHeight;
    }
}

void setWindowSize(int width, int height)
{
    normalizeWindowSize(width, height);

    assert(m_window);

    clampWindowSize(width, height);

    logInfo("Setting new window size: %dx%d", width, height);

    SDL_SetWindowSize(m_window, width, height);
    SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);

    m_windowWidth = width;
    m_windowHeight = height;
}

bool getWindowResizable()
{
    assert(m_window);
    return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_RESIZABLE) != 0;
}

WindowMode getWindowMode()
{
    return m_windowMode;
}

int getWindowDisplayIndex()
{
    assert(m_window);
    return SDL_GetWindowDisplayIndex(m_window);
}

bool isInFullScreenMode()
{
    return m_windowMode == kModeFullScreen;
}

std::pair<int, int> getFullScreenDimensions()
{
    return { m_displayWidth, m_displayHeight };
}

std::pair<int, int> getVisibleFieldSize()
{
    return { m_fieldWidth, m_fieldHeight };
}

int getVisibleFieldWidth()
{
    return m_fieldWidth;
}

static bool setDisplayMode(int width, int height)
{
    assert(m_window);

    logInfo("Trying to switch to %dx%d", width, height);

    SDL_DisplayMode mode;
    mode.w = width;
    mode.h = height;
    mode.refresh_rate = 0;
    mode.driverdata = nullptr;
    mode.format = SDL_PIXELFORMAT_RGB888;

    int err = SDL_SetWindowDisplayMode(m_window, &mode);
    if (err) {
        logWarn("SDL_SetWindowDisplayMode() failed with code: %d, reason: %s", err, SDL_GetError());
        return false;
    }

    if (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN)
        SDL_SetWindowFullscreen(m_window, 0);

    err = SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);

    if (!err) {
        if (SDL_GetWindowDisplayMode(m_window, &mode) == 0 && (mode.w != width || mode.h != height))
            logWarn("Couldn't switch to %dx%d, switched to %dx%d instead", width, height, mode.w, mode.h);

        m_displayWidth = mode.w;
        m_displayHeight = mode.h;
    } else {
        logWarn("SDL_SetWindowFullscreen() failed with code: %d, reason: %s", err, SDL_GetError());
    }

    return err == 0;
}

static void setWindowMode(WindowMode newMode)
{
    bool success = true;
    auto mode = "standard windowed";

    assert(m_window);

    if (m_windowMode != newMode)
        logInfo("Window mode change requested, from %d to %d", m_windowMode, newMode);

    switch (newMode) {
    case kModeWindow:
        success = SDL_SetWindowFullscreen(m_window, 0) == 0;
        if (success)
            SDL_SetWindowSize(m_window, m_windowWidth, m_windowHeight);
        break;

    case kModeBorderlessMaximized:
        success = SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;
        mode = "borderless maximized";
        break;

    case kModeFullScreen:
        success = setDisplayMode(m_displayWidth, m_displayHeight);
        mode = "full screen";
        break;

    default:
        assert(false);
    }

    m_windowMode = newMode;

    if (success) {
        logInfo("Successfully switched into %s mode", mode);
    } else {
        logWarn("Failed to change window mode to %s", mode);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Resolution switch failure", "Failed to change window mode", m_window);

        int flags = SDL_GetWindowFlags(m_window);
        m_windowMode = kModeWindow;

        if (flags & SDL_WINDOW_FULLSCREEN)
            m_windowMode = kModeFullScreen;
        else if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
            m_windowMode = kModeBorderlessMaximized;
    }
}

void switchToWindow()
{
    if (m_windowMode != kModeWindow)
        setWindowMode(kModeWindow);
}

void switchToBorderlessMaximized()
{
    if (m_windowMode != kModeBorderlessMaximized)
        setWindowMode(kModeBorderlessMaximized);
}

void setWindowResizable(bool resizable)
{
    assert(m_window);

    SDL_SetWindowResizable(m_window, resizable ? SDL_TRUE : SDL_FALSE);
    m_windowResizable = resizable;
}

bool setFullScreenResolution(int width, int height)
{
    bool result = setDisplayMode(width, height);
    if (result)
        m_windowMode = kModeFullScreen;

    return result;
}

void toggleBorderlessMaximizedMode()
{
    if (m_windowMode == kModeFullScreen || m_windowMode == kModeBorderlessMaximized)
        setWindowMode(kModeWindow);
    else
        setWindowMode(kModeBorderlessMaximized);
}

static std::pair<int, int> getDesktopResolution()
{
    auto displayIndex = SDL_GetWindowDisplayIndex(m_window);
    if (displayIndex >= 0) {
        SDL_DisplayMode mode;
        if (!SDL_GetDesktopDisplayMode(displayIndex, &mode))
            return { mode.w, mode.h };
    }

    return { kDefaultFullScreenWidth, kDefaultFullScreenHeight };
}

void toggleFullScreenMode()
{
    if (!m_displayWidth || !m_displayHeight)
        std::tie(m_displayWidth, m_displayHeight) = getDesktopResolution();

    if (m_windowMode == kModeFullScreen || m_windowMode == kModeBorderlessMaximized)
        setWindowMode(kModeWindow);
    else
        setWindowMode(kModeFullScreen);
}

void toggleWindowResizable()
{
    if (m_windowMode != kModeWindow)
        setWindowMode(kModeWindow);

    setWindowResizable(!m_windowResizable);
}

void centerWindow()
{
    assert(m_window);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

bool hasMouseFocus()
{
    assert(m_window);
    return SDL_GetMouseFocus() == m_window;
}

bool mapCoordinatesToGameArea(int &x, int &y)
{
    int windowWidth, windowHeight;

    SDL_Rect viewport = getViewport();

    if (isInFullScreenMode())
        std::tie(windowWidth, windowHeight) = getFullScreenDimensions();
    else
        std::tie(windowWidth, windowHeight) = getWindowSize();

    int slackWidth = 0;
    int slackHeight = 0;

    if (viewport.x) {
        auto logicalWidth = kVgaWidth * windowHeight / kVgaHeight;
        slackWidth = (windowWidth - logicalWidth) / 2;
    } else if (viewport.y) {
        auto logicalHeight = kVgaHeight * windowWidth / kVgaWidth;
        slackHeight = (windowHeight - logicalHeight) / 2;
    }

    if (y < slackHeight || y >= windowHeight - slackHeight)
        return false;

    if (x < slackWidth || x >= windowWidth - slackWidth)
        return false;

    x = (x - slackWidth) * kVgaWidth / (windowWidth - 2 * slackWidth);
    y = (y - slackHeight) * kVgaHeight / (windowHeight - 2 * slackHeight);

    return true;
}

#ifdef VIRTUAL_JOYPAD
bool getShowTouchTrails()
{
    return m_showTouchTrails;
}
void setShowTouchTrails(bool showTouchTrails)
{
    m_showTouchTrails = showTouchTrails;
}
bool getTransparentVirtualJoypadButtons()
{
    return m_transparentButtons;
}
void setTransparentVirtualJoypadButtons(bool transparentButtons)
{
    m_transparentButtons = transparentButtons;
}
#endif

bool cursorFlashingEnabled()
{
    return m_flashMenuCursor;
}

void setFlashMenuCursor(bool flashMenuCursor)
{
    m_flashMenuCursor = flashMenuCursor;
}

void loadVideoOptions(const CSimpleIniA& ini)
{
#ifdef __ANDROID__
    m_windowMode = kModeFullScreen;
#else
    m_windowMode = static_cast<WindowMode>(ini.GetLongValue(kVideoSection, kWindowMode, kModeWindow));

    if (m_windowMode >= kNumWindowModes)
        m_windowMode = kModeWindow;
#endif

    m_windowWidth = ini.GetLongValue(kVideoSection, kWindowWidthKey, kWindowWidth);
    m_windowHeight = ini.GetLongValue(kVideoSection, kWindowHeightKey, kWindowHeight);

    m_fieldWidth = ini.GetLongValue(kVideoSection, kFieldWidthKey, kGameScreenWidth);
    m_fieldHeight = ini.GetLongValue(kVideoSection, kFieldHeightKey, kVgaHeight);

    normalizeWindowSize(m_windowWidth, m_windowHeight);

    m_displayWidth = ini.GetLongValue(kVideoSection, kFullScreenWidth);
    m_displayHeight = ini.GetLongValue(kVideoSection, kFullScreenHeight);

    if (!m_displayWidth)
        m_displayWidth = kDefaultFullScreenWidth;
    if (!m_displayHeight)
        m_displayHeight = kDefaultFullScreenHeight;

    m_windowResizable = ini.GetLongValue(kVideoSection, kWindowResizable, 1) != 0;
    m_flashMenuCursor = ini.GetBoolValue(kVideoSection, kFlashMenuCursor, true);

#ifdef VIRTUAL_JOYPAD
    m_showTouchTrails = ini.GetBoolValue(kVideoSection, kShowTouchTrails, true);
    m_transparentButtons = ini.GetBoolValue(kVideoSection, kTransparentButtons, true);
#endif
}

void saveVideoOptions(CSimpleIniA& ini)
{
    ini.SetLongValue(kVideoSection, kWindowMode, static_cast<long>(m_windowMode));

    int windowWidth, windowHeight;
    std::tie(windowWidth, windowHeight) = getWindowSize();

    ini.SetLongValue(kVideoSection, kWindowWidthKey, windowWidth);
    ini.SetLongValue(kVideoSection, kWindowHeightKey, windowHeight);

    ini.SetLongValue(kVideoSection, kFullScreenWidth, m_displayWidth);
    ini.SetLongValue(kVideoSection, kFullScreenHeight, m_displayHeight);

    ini.SetLongValue(kVideoSection, kWindowResizable, m_windowResizable);

    ini.SetBoolValue(kVideoSection, kFlashMenuCursor, m_flashMenuCursor);

#ifdef VIRTUAL_JOYPAD
    ini.SetBoolValue(kVideoSection, kShowTouchTrails, m_showTouchTrails);
    ini.SetBoolValue(kVideoSection, kTransparentButtons, m_transparentButtons);
#endif
}
