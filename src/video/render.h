#pragma once

constexpr int kVgaWidth = 320;
constexpr int kVgaHeight = 200;
constexpr int kVgaScreenSize = kVgaWidth * kVgaHeight;

constexpr int kVgaPaletteNumColors = 256;
constexpr int kVgaPaletteSize = kVgaPaletteNumColors * 3;

void initRendering();
void finishRendering();
SDL_Rect getViewport();
void setPalette(const char *palette, int numColors = kVgaPaletteNumColors);
void getPalette(char *palette);
void clearScreen();
void skipFrameUpdate();
void updateScreen(const char *data = nullptr, int offsetLine = 0, int numLines = kVgaHeight);
void frameDelay(double factor = 1.0);
void timerProc();
void fadeIfNeeded();

void makeScreenshot();
