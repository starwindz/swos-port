#pragma once

constexpr int kVgaWidth = 320;
constexpr int kVgaHeight = 200;
constexpr int kVgaScreenSize = kVgaWidth * kVgaHeight;

constexpr int kVgaPaletteNumColors = 256;
constexpr int kVgaPaletteSize = kVgaPaletteNumColors * 3;

constexpr int kWindowWidth = 4 * kVgaWidth;
constexpr int kWindowHeight = 4 * kVgaHeight;

enum WindowMode { kModeFullScreen, kModeWindow, kModeBorderlessMaximized, kNumWindowModes, };

std::pair<int, int> getWindowSize();
void setWindowSize(int width, int height);
bool getWindowResizable();
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
void centerWindow();

bool hasMouseFocus();
SDL_Rect getViewport();

void loadVideoOptions(const CSimpleIni& ini);
void saveVideoOptions(CSimpleIni& ini);

void initRendering();
void finishRendering();
void setPalette(const char *palette, int numColors = kVgaPaletteNumColors);
void getPalette(char *palette);
void clearScreen();
void skipFrameUpdate();
void updateScreen(const char *data = nullptr, int offsetLine = 0, int numLines = kVgaHeight);
void frameDelay(double factor = 1.0);
void timerProc();

void showVideoOptionsMenu();
