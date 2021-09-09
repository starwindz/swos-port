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
void updateScreen(bool delay = false);

void fadeIn();
void fadeOut();
void fadeInAndOut(void (*callback)() = nullptr);

void drawRectangle(int x, int y, int width, int height, const Color& color);

bool getLinearFiltering();
void setLinearFiltering(bool useLinearFiltering);

void makeScreenshot();
