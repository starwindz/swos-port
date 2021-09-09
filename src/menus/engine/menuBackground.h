#pragma once

#include "render.h"

void initMenuBackground();
void drawMenuBackground(int offsetLine = 0, int numLines = kVgaHeight);
void setStandardMenuBackgroundImage();
void setMenuBackgroundImage(const char *name);
void unloadMenuBackground();
