#pragma once

enum WindowMode
{
    kModeFullScreen, kModeWindow, kModeBorderlessMaximized, kNumWindowModes,
};

enum class AssetResolution {
    k4k, kHD, kLowRes, kNumResolutions, kInvalid = kNumResolutions,
};

constexpr auto kNumAssetResolutions = static_cast<size_t>(AssetResolution::kNumResolutions);

using AssetResolutionChangeHandler = std::function<void(AssetResolution, AssetResolution)>;

SDL_Window *createWindow();
void destroyWindow();
void initWindow();
std::pair<int, int> getWindowSize();
AssetResolution getAssetResolution();
void registerAssetResolutionChangeHandler(AssetResolutionChangeHandler handler);
const char *getAssetDir();
std::string getPathInAssetDir(const char *path);
void setWindowSize(int width, int height);
bool getWindowResizable();
bool getWindowMaximized();
WindowMode getWindowMode();
int getWindowDisplayIndex();
bool setFullScreenResolution(int width, int height);
bool isInFullScreenMode();
std::pair<int, int> getFullScreenDimensions();

void switchToWindow();
void switchToBorderlessMaximized();

void toggleBorderlessMaximizedMode();
void toggleFullScreenMode();
void toggleWindowResizable();
void toggleWindowMaximized();
void centerWindow();

bool hasMouseFocus();
bool mapCoordinatesToGameArea(int& x, int& y);
float getGameScale();
float getGameScreenOffsetX();
float getGameScreenOffsetY();
float getFieldWidth();
float getFieldHeight();
SDL_FRect mapRect(int x, int y, int width, int height);

#ifdef VIRTUAL_JOYPAD
bool getShowTouchTrails();
void setShowTouchTrails(bool showTouchTrails);
bool getTransparentVirtualJoypadButtons();
void setTransparentVirtualJoypadButtons(bool transparentButtons);
#endif
bool cursorFlashingEnabled();
void setFlashMenuCursor(bool flashMenuCursor);
bool getShowFps();
void setShowFps(bool showFps);

void loadVideoOptions(const CSimpleIni& ini);
void saveVideoOptions(CSimpleIni& ini);
