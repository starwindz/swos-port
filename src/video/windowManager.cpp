#include "windowManager.h"
#include "render.h"
#include "pitch.h"
#include "util.h"
#include "file.h"

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 800;

constexpr int kDefaultFullScreenWidth = 640;
constexpr int kDefaultFullScreenHeight = 480;

constexpr int kMinimumWindowWidth= 640;
constexpr int kMinimumWindowHeight = 400;

static SDL_Window *m_window;
static auto m_windowMode =
#ifdef __ANDROID__
    kModeFullScreen;
#else
    kModeWindow;
#endif

static bool m_maximized;

static_assert(static_cast<int>(AssetResolution::kNumResolutions) == 3, "Update resolutions array");
struct AssetResolutionInfo {
    AssetResolution res;
    int width;
    int height;
};
static constexpr std::array<AssetResolutionInfo, 3> kAssetResolutionsInfo = {{
    { AssetResolution::kLowRes, 960, 540 },
    { AssetResolution::kHD, 1920, 1080 },
    { AssetResolution::k4k, 3840, 2160 },
}};

static AssetResolution m_resolution = AssetResolution::kLowRes;
static std::vector<AssetResolutionChangeHandler> m_resChangeHandlers;

static int m_windowWidth;
static int m_windowHeight;
static int m_nonMaximizedWidth;
static int m_nonMaximizedHeight;

// full-screen dimensions
static int m_displayWidth = kDefaultFullScreenWidth;
static int m_displayHeight = kDefaultFullScreenHeight;
static bool m_windowResizable = true;

static float m_gameScale;
static float m_gameOffsetX;
static float m_gameOffsetY;

static float m_fieldWidth;
static float m_fieldHeight;

static bool m_flashMenuCursor = true;
static bool m_showFps;

#ifdef VIRTUAL_JOYPAD
static bool m_showTouchTrails = true;
static bool m_transparentButtons = true;
#endif

// video options
const char kVideoSection[] = "video";
const char kWindowMode[] = "windowMode";
const char kWindowMaximized[] = "windowMaximized";
const char kWindowWidthKey[] = "windowWidth";
const char kWindowHeightKey[] = "windowHeight";
const char kWindowResizable[] = "windowResizable";
const char kFullScreenWidth[] = "fullScreenWidth";
const char kFullScreenHeight[] = "fullScreenHeight";
const char kFlashMenuCursor[] = "flashMenuCursor";
const char kShowFps[] = "showFps";
const char kUseLinearFiltering[] = "useLinearFiltering";
const char kClearScreen[] = "clearScreen";
#ifdef VIRTUAL_JOYPAD
const char kShowTouchTrails[] = "showTouchTrails";
const char kTransparentButtons[] = "virtualJoypadTransparentButtons";
#endif

static void updateLogicalScreenSize();
static void updateWindowDimensions(int width, int height, bool init = false);
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

    auto handleSizeChanged = [](void *, SDL_Event *event) {
        if (event->type == SDL_WINDOWEVENT) {
            switch (event->window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                updateWindowDimensions(event->window.data1, event->window.data2);
                break;
            case SDL_WINDOWEVENT_MAXIMIZED:
                m_maximized = true;
                break;
            case SDL_WINDOWEVENT_RESTORED:
                m_maximized = false;
                m_nonMaximizedWidth = m_windowWidth;
                m_nonMaximizedHeight = m_windowHeight;
                break;
            }
        }
        return 0;
    };

    SDL_AddEventWatch(handleSizeChanged, nullptr);

    return m_window;
}

void destroyWindow()
{
    if (m_window)
        SDL_DestroyWindow(m_window);
}

void initWindow()
{
    if (m_maximized)
        SDL_MaximizeWindow(m_window);
    else
        updateLogicalScreenSize();
}

SDL_Window *getWindow()
{
    return m_window;
}

static void updateAssetResolution()
{
    int minDiff = INT_MAX;
    auto oldResolution = m_resolution;
    m_resolution = AssetResolution::kLowRes;

    for (const auto& info : kAssetResolutionsInfo) {
        int diff = std::abs(info.width - m_windowWidth) + std::abs(info.height - m_windowHeight);
        if (diff < minDiff) {
            m_resolution = info.res;
            minDiff = diff;
        }
    }

    if (m_resolution != oldResolution)
        for (const auto& handler : m_resChangeHandlers)
            handler(oldResolution, m_resolution);
}

static void updateGameScaleFactor()
{
    auto scaleX = static_cast<float>(m_windowWidth) / kVgaWidth;
    auto scaleY = static_cast<float>(m_windowHeight) / kVgaHeight;
    m_gameScale = std::min(scaleX, scaleY);
}

static void updateLogicalScreenSize()
{
    m_fieldWidth = m_windowWidth / m_gameScale;
    m_fieldHeight = m_windowHeight / m_gameScale;

    auto renderer = getRenderer();
    if (m_fieldWidth > kPitchWidth || m_fieldHeight > kPitchHeight) {
        m_fieldWidth = std::min(m_fieldWidth, static_cast<float>(kPitchWidth));
        m_fieldHeight = std::min(m_fieldHeight, static_cast<float>(kPitchHeight));
        SDL_RenderSetLogicalSize(renderer, std::lroundf(m_fieldWidth * m_gameScale), std::lroundf(m_fieldHeight * m_gameScale));
    } else {
        SDL_RenderSetLogicalSize(renderer, 0, 0);
    }

    m_gameOffsetX = (m_fieldWidth - kVgaWidth) / 2 * m_gameScale;
    m_gameOffsetY = (m_fieldHeight - kVgaHeight) / 2 * m_gameScale;

    assert(m_gameOffsetX >= -0.001 && m_gameOffsetY >= -0.001);
}

static void updateWindowDimensions(int width, int height, bool init /* = false */)
{
    if (width != m_windowWidth || height != m_windowHeight) {
        m_windowWidth = width;
        m_windowHeight = height;
        updateAssetResolution();
        updateGameScaleFactor();
        if (!init)
            updateLogicalScreenSize();
    }
}

std::pair<int, int> getWindowSize()
{
    assert(m_window);

    int width = -1, height = -1;

    if (m_windowMode == kModeWindow || m_windowMode == kModeBorderlessMaximized) {
        SDL_GetWindowSize(m_window, &width, &height);

        if (width > 0 && height > 0) {
            if (m_windowMode == kModeWindow)
                updateWindowDimensions(width, height);

            return { width, height };
        }
    }

    return { m_windowWidth, m_windowHeight };
}

AssetResolution getAssetResolution()
{
    return m_resolution;
}

void registerAssetResolutionChangeHandler(AssetResolutionChangeHandler handler)
{
    m_resChangeHandlers.push_back(handler);
}

const char *getAssetDir()
{
    static_assert(static_cast<int>(AssetResolution::kNumResolutions) == 3, "Somewhere out there in the space");

    switch (m_resolution) {
    case AssetResolution::k4k: return "assets" DIR_SEPARATOR "4k";
    case AssetResolution::kHD: return "assets" DIR_SEPARATOR "hd";
    case AssetResolution::kLowRes: return "assets" DIR_SEPARATOR "low-res";
    }

    return nullptr;
}

std::string getPathInAssetDir(const char *path)
{
    return std::string(getAssetDir()) + getDirSeparator() + path;
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

static void normalizeWindowSize(int& width, int& height, int defaultWidth, int defaultHeight)
{
    if (width < kMinimumWindowWidth || height <= kMinimumWindowHeight) {
        logInfo("Got invalid width/height (%dx%d), setting to default (%dx%d)", width, height, defaultWidth, defaultHeight);
        width = defaultWidth;
        height = defaultHeight;
    }
}

void setWindowSize(int width, int height)
{
    normalizeWindowSize(width, height, kWindowWidth, kWindowHeight);

    assert(m_window);

    clampWindowSize(width, height);

    logInfo("Setting new window size: %dx%d", width, height);

    SDL_SetWindowSize(m_window, width, height);
    SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);

    updateWindowDimensions(width, height);
}

bool getWindowResizable()
{
    assert(m_window);
    return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_RESIZABLE) != 0;
}

bool getWindowMaximized()
{
    assert(m_window);
    return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_MAXIMIZED) != 0;
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

void toggleWindowMaximized()
{
    assert(m_window);

    if (getWindowMaximized())
        SDL_RestoreWindow(m_window);
    else
        SDL_MaximizeWindow(m_window);
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

    if (isInFullScreenMode()) {
        std::tie(windowWidth, windowHeight) = getFullScreenDimensions();
    } else {
        auto viewPort = getViewport();
        x -= viewPort.x;
        y -= viewPort.y;
    }

    auto scale = getGameScale();
    auto xOffset = getGameScreenOffsetX();
    auto yOffset = getGameScreenOffsetY();

    if (x < xOffset || y < yOffset)
        return false;

    x = static_cast<int>(std::round((x - xOffset) / scale));
    y = static_cast<int>(std::round((y - yOffset) / scale));

    return true;
}

float getGameScale()
{
    return m_gameScale;
}

float getGameScreenOffsetX()
{
    return m_gameOffsetX;
}

float getGameScreenOffsetY()
{
    return m_gameOffsetY;
}

float getFieldWidth()
{
    return m_fieldWidth;
}

float getFieldHeight()
{
    return m_fieldHeight;
}

SDL_FRect mapRect(int x, int y, int width, int height)
{
    return { m_gameOffsetX + x * m_gameScale, m_gameOffsetY + y * m_gameScale, width * m_gameScale, height * m_gameScale };
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

bool getShowFps()
{
    return m_showFps;
}

void setShowFps(bool showFps)
{
    m_showFps = showFps;
}

void loadVideoOptions(const CSimpleIniA& ini)
{
#ifdef __ANDROID__
    m_windowMode = kModeFullScreen;
#else
    m_windowMode = static_cast<WindowMode>(ini.GetLongValue(kVideoSection, kWindowMode, kModeWindow));

    if (m_windowMode >= kNumWindowModes)
        m_windowMode = kModeWindow;

    m_maximized = ini.GetBoolValue(kVideoSection, kWindowMaximized);
#endif

    int width = ini.GetLongValue(kVideoSection, kWindowWidthKey, kWindowWidth);
    int height = ini.GetLongValue(kVideoSection, kWindowHeightKey, kWindowHeight);

    normalizeWindowSize(width, height, kWindowWidth, kWindowHeight);
    updateWindowDimensions(width, height, true);

    m_nonMaximizedWidth = m_windowWidth;
    m_nonMaximizedHeight = m_windowHeight;

    m_displayWidth = ini.GetLongValue(kVideoSection, kFullScreenWidth);
    m_displayHeight = ini.GetLongValue(kVideoSection, kFullScreenHeight);

    normalizeWindowSize(m_displayWidth, m_displayHeight, kDefaultFullScreenWidth, kDefaultFullScreenHeight);

    m_windowResizable = ini.GetLongValue(kVideoSection, kWindowResizable, 1) != 0;
    m_flashMenuCursor = ini.GetBoolValue(kVideoSection, kFlashMenuCursor, true);
    m_showFps = ini.GetBoolValue(kVideoSection, kShowFps, false);
    setLinearFiltering(ini.GetBoolValue(kVideoSection, kUseLinearFiltering, false));
    setClearScreen(ini.GetBoolValue(kVideoSection, kClearScreen, true));

#ifdef VIRTUAL_JOYPAD
    m_showTouchTrails = ini.GetBoolValue(kVideoSection, kShowTouchTrails, true);
    m_transparentButtons = ini.GetBoolValue(kVideoSection, kTransparentButtons, true);
#endif
}

void saveVideoOptions(CSimpleIniA& ini)
{
    ini.SetLongValue(kVideoSection, kWindowMode, static_cast<long>(m_windowMode));
    ini.SetBoolValue(kVideoSection, kWindowMaximized, m_maximized);

    int windowWidth, windowHeight;

    if (m_windowMode == kModeWindow && m_maximized) {
        windowWidth = m_nonMaximizedWidth;
        windowHeight = m_nonMaximizedHeight;
    } else {
        std::tie(windowWidth, windowHeight) = getWindowSize();
    }

    ini.SetLongValue(kVideoSection, kWindowWidthKey, windowWidth);
    ini.SetLongValue(kVideoSection, kWindowHeightKey, windowHeight);

    ini.SetLongValue(kVideoSection, kFullScreenWidth, m_displayWidth);
    ini.SetLongValue(kVideoSection, kFullScreenHeight, m_displayHeight);

    ini.SetLongValue(kVideoSection, kWindowResizable, m_windowResizable);

    ini.SetBoolValue(kVideoSection, kFlashMenuCursor, m_flashMenuCursor);
    ini.SetBoolValue(kVideoSection, kShowFps, m_showFps);
    ini.SetBoolValue(kVideoSection, kUseLinearFiltering, getLinearFiltering());
    ini.SetBoolValue(kVideoSection, kClearScreen, getClearScreen());

#ifdef VIRTUAL_JOYPAD
    ini.SetBoolValue(kVideoSection, kShowTouchTrails, m_showTouchTrails);
    ini.SetBoolValue(kVideoSection, kTransparentButtons, m_transparentButtons);
#endif
}
