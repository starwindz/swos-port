#include "render.h"
#include "mockRender.h"

static UpdateHook m_updateHook;

void setUpdateHook(UpdateHook updateHook)
{
    m_updateHook = updateHook;
}

void updateScreen(const char *, int, int)
{
    if (m_updateHook)
        m_updateHook();
}

void initRendering() {}
void finishRendering() {}
SDL_Rect getViewport() { return {}; }
void setPalette(const char *, int) {}
void getPalette(char *) {}
void clearScreen() {}
void skipFrameUpdate() {}
void frameDelay(double) {}
void timerProc() {}
void fadeIfNeeded() {}
void makeScreenshot() {}

void SWOS::Flip() {}
