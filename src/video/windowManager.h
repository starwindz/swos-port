#pragma once

#include "render.h"

constexpr int kWindowWidth = 4 * kVgaWidth;
constexpr int kWindowHeight = 4 * kVgaHeight;

enum WindowMode
{
    kModeFullScreen, kModeWindow, kModeBorderlessMaximized, kNumWindowModes,
};

SDL_Window *createWindow();
void destroyWindow();
std::pair<int, int> getWindowSize();
void setWindowSize(int width, int height);
bool getWindowResizable();
WindowMode getWindowMode();
int getWindowDisplayIndex();
bool setFullScreenResolution(int width, int height);
bool isInFullScreenMode();
std::pair<int, int> getFullScreenDimensions();
std::pair<int, int> getVisibleFieldSize();
int getVisibleFieldWidth();

void switchToWindow();
void switchToBorderlessMaximized();

void toggleBorderlessMaximizedMode();
void toggleFullScreenMode();
void toggleWindowResizable();
void centerWindow();

bool hasMouseFocus();
bool mapCoordinatesToGameArea(int& x, int& y);

#ifdef VIRTUAL_JOYPAD
bool getShowTouchTrails();
void setShowTouchTrails(bool showTouchTrails);
bool getTransparentVirtualJoypadButtons();
void setTransparentVirtualJoypadButtons(bool transparentButtons);
#endif
bool cursorFlashingEnabled();
void setFlashMenuCursor(bool flashMenuCursor);

void loadVideoOptions(const CSimpleIni& ini);
void saveVideoOptions(CSimpleIni& ini);
