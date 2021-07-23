#include "gameTime.h"
#include "gameSprites.h"
#include "sprites.h"
#include "renderSprites.h"

using LastPeriodMinuteHandler = void (*)();
using TimeDigitSprites = std::array<int, 4>;

static int m_endGameCounter;

static bool isGameAtMinute(dword minute);
static bool prolongLastMinute();
static void positionTimeSprite();
static TimeDigitSprites getGameTimeSprites();
static LastPeriodMinuteHandler getPeriodEndHandler();
static bool isNextMinuteLastInPeriod();
static void bumpGameTime();
static void setupLastMinuteSwitchNextFrame();
static void ensureTimeSpriteIsVisible();

void resetTime()
{
    swos.gameSeconds = 0;
    memset(&swos.gameTime, 0, sizeof(swos.gameTime));
    swos.secondsSwitchAccumulator = 0;
    positionTimeSprite();
    swos.currentTimeSprite.visible = 0;
    swos.currentTimeSprite.pictureIndex = kTimeSprite118Mins;
}

void updateGameTime()
{
    ensureTimeSpriteIsVisible();

    bool minuteSwitchAboutToHappen = swos.gameSeconds < 0;

    if (minuteSwitchAboutToHappen) {
        m_endGameCounter -= swos.timerDifference;
        if (m_endGameCounter < 0) {
            if (auto periodEndHandler = getPeriodEndHandler()) {
                swos.gameSeconds = 0;
                swos.stateGoal = 0;
                SWOS::PlayEndGameWhistleSample();
                periodEndHandler();
            }
            positionTimeSprite();
        } else {
            if (prolongLastMinute()) {
                setupLastMinuteSwitchNextFrame();
                positionTimeSprite();
            }
        }
    } else if (swos.gameStatePl == GameState::kInProgress && !swos.playingPenalties) {
        swos.secondsSwitchAccumulator -= swos.timeDelta;
        if (swos.secondsSwitchAccumulator < 0) {
            swos.secondsSwitchAccumulator += 54;
            swos.gameSeconds += swos.timerDifference;

            if (swos.gameSeconds >= 60) {
                swos.gameSeconds = 0;
                bumpGameTime();

                if (isGameAtMinute(1) || isGameAtMinute(46))
                    BumpAllPlayersLastPlayedHalfAtGameStart();

                if (isNextMinuteLastInPeriod()) {
                    setupLastMinuteSwitchNextFrame();
                    positionTimeSprite();
                } else {
                    positionTimeSprite();
                }
            }
        }
    }

    positionTimeSprite();
}

void drawGameTime(const Sprite& sprite)
{
    assert(!sprite.x.fraction() && !sprite.y.fraction() && !sprite.z.fraction());

    int x = sprite.x.whole();
    int y = sprite.y.whole();
    int z = sprite.z.whole();

    auto kDigitWidth = getSprite(kBigTimeDigitSprite0 + 8).width;
    auto timeDigitSprites = getGameTimeSprites();

    int xOffset = 0;
    for (size_t i = 0; i < timeDigitSprites.size() && timeDigitSprites[i] >= 0; i++) {
        drawMenuSprite(timeDigitSprites[i], x + xOffset, y - z);
        xOffset += kDigitWidth;
    }

    drawMenuSprite(kTimeSprite8Mins, x + xOffset, y - z);
}

static dword gameTimeInMinutes()
{
    return swos.gameTime[1] * 100 + swos.gameTime[2] * 10 + swos.gameTime[3];
}

static bool isGameAtMinute(dword minute)
{
    return minute == gameTimeInMinutes();
}

static void endFirstHalf()
{
    EndFirstHalf();
    BumpAllPlayersLastPlayedHalfPostGame();
}

static void endSecondHalf()
{
    BumpAllPlayersLastPlayedHalfPostGame();
    swos.statsTeam1GoalsCopy = swos.statsTeam1Goals;
    swos.statsTeam2GoalsCopy = swos.statsTeam2Goals;

    int totalTeam1Goals = swos.team1TotalGoals;
    int totalTeam2Goals = swos.team2TotalGoals;

    if (totalTeam1Goals == totalTeam2Goals) {
        totalTeam1Goals = swos.statsTeam1Goals + 2 * swos.team1GoalsFirstLeg;
        totalTeam2Goals = swos.statsTeam2Goals + 2 * swos.team2GoalsFirstLeg;

        bool gameTied = !swos.secondLeg || swos.playing2ndGame != 1 || totalTeam1Goals == totalTeam2Goals;
        if (gameTied) {
            if (swos.extraTimeState) {
                swos.extraTimeState = -1;
                StartFirstExtraTime();
            } else if (swos.penaltiesState) {
                swos.penaltiesState = -1;
                StartPenalties();
            } else {
                swos.winningTeamPtr = nullptr;
                EndOfGame();
            }
            return;
        }
    }

    swos.winningTeamPtr = totalTeam1Goals > totalTeam2Goals ? &swos.topTeamIngame : &swos.bottomTeamIngame;
    EndOfGame();
}

static bool prolongLastMinute()
{
    if (swos.gameStatePl != GameState::kInProgress)
        return true;

    constexpr int kUpperPenaltyAreaLowerLine = 216;
    constexpr int kLowerPenaltyAreaUpperLine = 682;
    constexpr int kCenterLine = 449;

    auto ballY = swos.ballSprite.y;
    auto ballInsidePenaltyArea = ballY <= kUpperPenaltyAreaLowerLine || ballY > kLowerPenaltyAreaUpperLine;
    auto attackingTeam = ballY > kCenterLine ? &swos.topTeamData : &swos.bottomTeamData;
    auto attackInProgress = swos.lastTeamPlayed == attackingTeam;

    return ballInsidePenaltyArea || attackInProgress;
}

static void endFirstExtraTime()
{
    EndFirstExtraTime();
}

static void endSecondExtraTime()
{
    int totalTeam1Goals = swos.team1TotalGoals;
    int totalTeam2Goals = swos.team2TotalGoals;

    if (totalTeam1Goals == totalTeam2Goals) {
        totalTeam1Goals = swos.statsTeam1Goals + 2 * swos.team1GoalsFirstLeg;
        totalTeam2Goals = swos.statsTeam2Goals + 2 * swos.team2GoalsFirstLeg;

        bool gameTied = !swos.secondLeg || !swos.playing2ndGame || totalTeam1Goals == totalTeam2Goals;
        if (gameTied) {
            if (swos.penaltiesState) {
                swos.penaltiesState = -1;
                StartPenalties();
            } else {
                swos.winningTeamPtr = nullptr;
                EndOfGame();
            }
            return;
        }
    }

    swos.winningTeamPtr = totalTeam1Goals > totalTeam2Goals ? &swos.topTeamIngame : &swos.bottomTeamIngame;
    EndOfGame();
}

static TimeDigitSprites getGameTimeSprites()
{
    TimeDigitSprites sprites = { -1, -1, -1, -1 };

    if (swos.gameTime[1]) {
        sprites[0] = swos.gameTime[1] + kBigTimeDigitSprite0;
        sprites[1] = swos.gameTime[2] + kBigTimeDigitSprite0;
        sprites[2] = swos.gameTime[3] + kBigTimeDigitSprite0;
    } else if (swos.gameTime[2]) {
        sprites[0] = swos.gameTime[2] + kBigTimeDigitSprite0;
        sprites[1] = swos.gameTime[3] + kBigTimeDigitSprite0;
    } else {
        sprites[0] = swos.gameTime[3] + kBigTimeDigitSprite0;
    }

    return sprites;
}

static LastPeriodMinuteHandler getPeriodEndHandler()
{
    switch (gameTimeInMinutes()) {
    case 45:  return endFirstHalf;
    case 90:  return endSecondHalf;
    case 105: return endFirstExtraTime;
    case 120: return endSecondExtraTime;
    default: return nullptr;
    }
}

static bool isNextMinuteLastInPeriod()
{
    return getPeriodEndHandler() != nullptr;
}

static void positionTimeSprite()
{
    constexpr int kTimeXOffset = 20 - 6;

    swos.currentTimeSprite.x = kTimeXOffset;

    constexpr int kTimeZOffset = 10'000;
    constexpr int kTimeYOffset = 9;

    swos.currentTimeSprite.y = kTimeYOffset + kTimeZOffset;
    swos.currentTimeSprite.z = kTimeZOffset;
}

static void bumpGameTime()
{
    if (++swos.gameTime[3] >= 10) {
        swos.gameTime[3] = 0;
        if (++swos.gameTime[2] >= 10) {
            swos.gameTime[2] = 0;
            if (swos.gameTime[1] < 9)
                swos.gameTime[1]++;
        }
    }
}

static void setupLastMinuteSwitchNextFrame()
{
    swos.gameSeconds = -1;
    m_endGameCounter = 55;
}

static void ensureTimeSpriteIsVisible()
{
    if (!swos.currentTimeSprite.visible) {
        swos.currentTimeSprite.visible = true;
        initDisplaySprites();
    }
}
