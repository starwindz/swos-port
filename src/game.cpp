#include "render.h"
#include "audio.h"
#include "log.h"
#include "controls.h"
#include "menu.h"
#include "dump.h"
#include "replays.h"
#include "sprites.h"
#include "replayExit.mnu.h"

void SWOS::InitGame_OnEnter()
{
    // soon after this SWOS will load the game palette, trashing old screen contents in the process
    // unlike the original game, which only changes palette register on the VGA card, we need those bytes
    // to persist in order to fade-out properly; so save it here so we can restore them later
    logInfo("Initializing the game...");
    memcpy(linAdr384k + 2 * kVgaScreenSize, linAdr384k, kVgaScreenSize);
}

__declspec(naked) void SWOS::ClearBackground_OnEnter()
{
    // fix SWOS bug, just in case
    _asm {
        xor edx, edx
        retn
    }
}

void SWOS::MainKeysCheck_OnEnter()
{
    if (testForPlayerKeys()) {
        lastKey = 0;
        return;
    }

    switch (convertedKey) {
    case 'D':
        toggleDebugOutput();
        break;
    case 'F':
        toggleFastReplay();
        break;
    }

    if (lastKey == kKeyF2)
        makeScreenshot();
}

static void darkenRectangle(int spriteIndex, int x, int y)
{
    D0 = spriteIndex;
    D1 = x;
    D2 = y;
    DarkenRectangle();
}

static void drawSprites(bool saveHihglightCoordinates = true)
{
    SortSprites();

    if (!numSpritesToRender)
        return;

    for (size_t i = 0; i < numSpritesToRender; i++) {
        auto player = sortedSprites[i];

        if (player->pictureIndex < 0)
            continue;

        auto sprite = spritesIndex[player->pictureIndex];

        int x = player->x - sprite->centerX - g_cameraX;
        int y = player->y - sprite->centerY - g_cameraY - player->z;

        SpriteClipper clipper(sprite, x, y);
        if (clipper.clip())
            clipper.clip();

        if (x < 336 && y < 200 && x > -sprite->width && y > -sprite->height) {
        //if (!clipper.clip()) {
            if (saveHihglightCoordinates && player != &big_S_Sprite && player->teamNumber)
                saveCoordinatesForHighlights(player->pictureIndex, x, y);

            player->beenDrawn = 1;

            if (player == &big_S_Sprite)
                deltaColor = 0x70;

            if (player->pictureIndex == kSquareGridForResultSpriteIndex)
                darkenRectangle(kSquareGridForResultSpriteIndex, x, y);
            else
                //drawSpriteUnclipped(clipper);
                drawSprite(player->pictureIndex, x, y);
        } else {
            player->beenDrawn = 0;
        }

        deltaColor = 0;
    }
}

static bool fadeOutInitiated;
static bool startingReplay;
static bool gameDone;

void SWOS::FadeOutToBlack_OnEnter()
{
    fadeOutInitiated = true;
}

void SWOS::HandleInstantReplayStateSwitch_OnEnter()
{
    if (!replayState)
        startingReplay = true;
}

void SWOS::GameOver_OnEnter()
{
    gameDone = true;
}

static void swosSetPalette(const char *palette)
{
    setPalette(palette);
    updateControls();
    frameDelay(0.5);

    // if the game is running, we have to redraw the sprites explicitly
    if (screenWidth == kGameScreenWidth) {
        // do one redraw of the sprites if the game was aborted, ended, or the replay is just starting,
        // but skip forced screen update if playing highlights
        if (fadeOutInitiated && !playHighlightFlag && (gameAborted || startingReplay || gameDone)) {
            // also be careful not to mess with saved highlights coordinates
            drawSprites(false);
            gameAborted = 0;
            fadeOutInitiated = false;
            startingReplay = false;
            gameDone = false;
        }
    }

    updateScreen();
}

// called from SWOS, with palette in esi
__declspec(naked) void SWOS::SetPalette()
{
    __asm {
        push ebx
        push esi
        push ecx    ; used by FadeOut

        push esi
        call swosSetPalette

        add esp, 4
        pop  ecx
        pop  esi
        pop  ebx
        retn
    }
}

void SWOS::DrawSprites()
{
    drawSprites();
}

void SWOS::EndProgram()
{
    std::exit(EXIT_FAILURE);
}

void SWOS::StartMainGameLoop()
{
    initGameControls();
    initNewReplay();

    A5 = &leftTeamIngame;
    A6 = &rightTeamIngame;

    EGA_graphics = 0;

    SAFE_INVOKE(GameLoop);

    vsPtr = linAdr384k;

    finishGameControls();
}

void SWOS::GameEnded()
{
    finishCurrentReplay();
}

// Fix crash when watching 2 CPU players with at least one top-class goalkeeper in the game.
// Goalkeeper skill is scaled to range 0..7 (in D0) but value clamping is skipped in CPU vs CPU mode.
void SWOS::FixTwoCPUsGameCrash()
{
    if (D0.asInt16() < 0)
        D0 = 0;
    if (D0.asInt16() > 7)
        D0 = 7;
}

//
// Replay/Exit menu after the end of a friendly game
//

void SWOS::ReplayExitMenuAfterFriendly()
{
    showMenu(replayExitMenu);
}

static void replayExitMenuOnInit()
{
    replaySelected = 0;

    DrawMenu();     // redraw menu so it's ready for the fade-in
    fadeIfNeeded();

    g_cameraX = 0;
    g_cameraY = 0;
}

static void replayExitMenuDone(bool replay = false)
{
    replaySelected = replay;
    SAFE_INVOKE(FadeOutToBlack);
    SetExitMenuFlag();

    if (replay)
        initNewReplay();
}

static void replayOnSelect()
{
    logInfo("Going for another one!");
    replayExitMenuDone(true);
}

static void exitOnSelect()
{
    replayExitMenuDone();
}
