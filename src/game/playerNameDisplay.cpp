// Handling of current player indicator displayed top-left (number + surname).
#include "playerNameDisplay.h"
#include "benchControls.h"
#include "camera.h"
#include "text.h"

static void hideCurrentPlayerName();
static void showNameBlinking(const Sprite *lastPlayer, const TeamGeneralInfo *lastTeam);
static void prolongLastPlayersName(const Sprite *lastPlayer, const TeamGeneralInfo *lastTeam);
static void resetNobodysBallTimer();

static void getPlayerNumberAndSurname(char *buf, const PlayerGame& player)
{
    SDL_itoa(player.shirtNumber, buf, 10);
    int shirtNumberLen = strlen(buf);
    buf[shirtNumberLen] = ' ';

    A0 = player.shortName;
    D0 = 0x4000 | 11;   // copy surname only, 11 characters max.
    GetSurname();

    strcpy(buf + shirtNumberLen + 1, A0.asPtr());
}

void setCurrentPlayerName()
{
    static int s_nobodysBallLastFrame;

    swos.currentPlayerNameSprite.show();

    if (swos.currentScorer) {
        showNameBlinking(swos.currentScorer, swos.lastTeamScored);
    } else if (swos.whichCard) {
        showNameBlinking(swos.bookedPlayer, swos.lastTeamBooked);
    } else if (swos.lastPlayerBeforeGoalkeeper) {
        prolongLastPlayersName(swos.lastPlayerBeforeGoalkeeper, swos.lastTeamScored);
    } else {
        auto lastPlayer = swos.lastPlayerPlayed;
        auto lastTeam = swos.lastTeamPlayed;

        if (swos.gameStatePl == GameState::kInProgress) {
            if (lastPlayer) {
                if (lastTeam->playerHasBall)
                    resetNobodysBallTimer();
                prolongLastPlayersName(lastPlayer, lastTeam);
            } else {
                hideCurrentPlayerName();
            }
        } else if (!lastPlayer || !lastTeam->controlledPlayerSprite) {
            s_nobodysBallLastFrame = 1;
            hideCurrentPlayerName();
        } else if (s_nobodysBallLastFrame) {
            s_nobodysBallLastFrame--;
            hideCurrentPlayerName();
        } else {
            resetNobodysBallTimer();
            prolongLastPlayersName(lastPlayer, lastTeam);
        }
    }
}

void drawPlayerName(int pictureIndex)
{
    assert(pictureIndex >= kTeam1PlayerNamesStartSprite && pictureIndex <= kTeam2PlayerNamesEndSprite);

    auto team1Player = pictureIndex < kTeam2PlayerNamesStartSprite;

    auto team = team1Player ? swos.topTeamPtr : swos.bottomTeamPtr;
    int playerIndex = pictureIndex - (team1Player ? kTeam1PlayerNamesStartSprite : kTeam2PlayerNamesStartSprite);
    const auto& player = team->players[playerIndex];

    char nameBuf[64];
    getPlayerNumberAndSurname(nameBuf, player);

    drawText(12, 0, nameBuf);
}

static void showCurrentPlayerName(const Sprite *lastPlayer, const TeamGeneralInfo *lastTeam)
{
    assert(lastPlayer && lastTeam);

    bool topTeam = lastTeam->inGameTeamPtr.asAligned() == &swos.topTeamIngame;
    int startSpriteIndex = topTeam ? kTeam1PlayerNamesStartSprite : kTeam2PlayerNamesStartSprite;

    int index = lastPlayer->playerOrdinal - 1;
    int playerOrdinal = getBenchPlayerShirtNumber(topTeam, index);
    int spriteIndex = startSpriteIndex + playerOrdinal;

    constexpr int kPlayerNameZ = 8'000;

    swos.currentPlayerNameSprite.pictureIndex = spriteIndex;
    swos.currentPlayerNameSprite.x = getCameraX();
    swos.currentPlayerNameSprite.y = getCameraY() + kPlayerNameZ;
    swos.currentPlayerNameSprite.z = kPlayerNameZ;
}

static void hideCurrentPlayerName()
{
    constexpr int kOverTheHillsAndFarAway = 25'000;

    swos.currentPlayerNameSprite.pictureIndex = -1;
    swos.currentPlayerNameSprite.x = kOverTheHillsAndFarAway;
    swos.currentPlayerNameSprite.y = kOverTheHillsAndFarAway;
    swos.currentPlayerNameSprite.z = kOverTheHillsAndFarAway;

    swos.lastPlayerBeforeGoalkeeper = nullptr;
}

static void showNameBlinking(const Sprite *lastPlayer, const TeamGeneralInfo *lastTeam)
{
    bool showName = (swos.stoppageTimer & 8) != 0;
    showName ? showCurrentPlayerName(lastPlayer, lastTeam) : hideCurrentPlayerName();
}

static void prolongLastPlayersName(const Sprite *lastPlayer, const TeamGeneralInfo *lastTeam)
{
    if (!swos.nobodysBallTimer) {
        hideCurrentPlayerName();
    } else {
        swos.nobodysBallTimer--;
        showCurrentPlayerName(lastPlayer, lastTeam);
    }
}

static void resetNobodysBallTimer()
{
    constexpr int kFramesBeforeNobodysBall = 50;

    // draw controlled player name for some time after the ball's gone
    swos.nobodysBallTimer = kFramesBeforeNobodysBall;
}
