//
// Replay/Exit menu after the end of a friendly game
//

#include "game.h"
#include "drawMenu.h"
#include "render.h"
#include "replays.h"
#include "replayExit.mnu.h"

static bool m_replaySelected;

bool showReplayExitMenuAfterFriendly()
{
    showMenu(replayExitMenu);
    return m_replaySelected;
}

static void replayExitMenuOnInit()
{
    m_replaySelected = false;

    drawMenu(false);    // redraw the menu so it's ready for the fade-in
    fadeIn();

    SDL_ShowCursor(SDL_ENABLE);
}

static void replayExitMenuDone(bool replay)
{
    m_replaySelected = replay;

    if (replay)
        initNewReplay();

    drawMenu(false);
    fadeOut();

    SetExitMenuFlag();

    updateCursor(replay);
}

static void replayOnSelect()
{
    logInfo("Going for another one!");
    replayExitMenuDone(true);
}

static void exitOnSelect()
{
    replayExitMenuDone(false);
}
