#pragma once

constexpr int kVgaWidth = 320;
constexpr int kVgaHeight = 200;
constexpr int kVgaScreenSize = kVgaWidth * kVgaHeight;

struct Color;

void initRendering();
void finishRendering();
SDL_Renderer *getRenderer();
SDL_Rect getViewport();
void skipFrameUpdate();
void updateScreen();
void updateFrame();
void frameDelay(double factor = 1.0);
void timerProc();
void fadeIfNeeded();

void drawFrame(int x, int y, int width, int height, const Color& color);

bool getLinearFiltering();
void setLinearFiltering(bool useLinearFiltering);

void makeScreenshot();
