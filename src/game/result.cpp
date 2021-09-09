#include "result.h"
#include "gameSprites.h"
#include "gameTime.h"
#include "camera.h"
#include "text.h"

constexpr int kMaxScorersForDisplay = 8;
constexpr int kMaxGoalsPerScorer = 10;

constexpr int kMaxScorerLineWidth = 120;
constexpr int kScorerLineHeight = 7;

constexpr int kCharactersPerLine = 61;  // theoretical maximum (all characters 2 pixels)

constexpr int kResultX = 160;
constexpr int kDashX = 157;
constexpr int kDashY = 249;

constexpr int kLeftMargin = 128;
constexpr int kRightMargin = 192;
constexpr int kRightGridX = 152;

constexpr int kBigLeftResultDigitX = 143;
constexpr int kBigLeftResultDigitY = 241;

constexpr int kSmallLeftResultDigitX = 150;
constexpr int kSmallRightResultDigitX = 163;
constexpr int kBigRightResultSecondDigitX = 165;

constexpr int kFirstLineBelowResultY = 263;
constexpr int kTeamNameY = 246;

constexpr int kResultBigDigitWidth = 12;
constexpr int kResultSmallDigitWidth = 6;
constexpr int kResultDigit1Offset = 4;

constexpr int kResultAtHalfTimeLength = 275;
constexpr int kResultAtGameBreakLength = 265;

constexpr int kEndOfHalfResult = 30'000;
constexpr int kGameBreakResult = 31'000;
constexpr int kMaxResultTicks = 32'000;
constexpr int kResultTickClamped = 29'000;

constexpr int kResultTextZ = 10'000;
constexpr int kResultGridZ = 9'000;
constexpr int kSmallDigitZ = 10'002;

struct GoalInfo
{
    GoalType type;
    char time[3];

    void update(GoalType goalType, const std::tuple<int, int, int>& gameTime) {
        type = goalType;
        time[0] = std::get<0>(gameTime) + '0';
        time[1] = std::get<1>(gameTime) + '0';
        time[2] = std::get<2>(gameTime) + '0';
    }
    int storeTime(char *buf) const {
        int i = 0;

        if (time[0] != '0')
            buf[i++] = time[0];
        if (time[1] != '0')
            buf[i++] = time[1];
        buf[i++] = time[2];

        return i;
    }
};

struct ScorerInfo
{
    int shirtNum;
    int numGoals;
    int numLines;
    std::array<GoalInfo, kMaxGoalsPerScorer> goals;
};

static std::array<ScorerInfo, kMaxScorersForDisplay> m_team1Scorers;
static std::array<ScorerInfo, kMaxScorersForDisplay> m_team2Scorers;

using ScorerLines = std::array<std::array<char, kCharactersPerLine>, kMaxScorersForDisplay>;
static ScorerLines m_team1ScorerLines;
static ScorerLines m_team2ScorerLines;

static const char *m_team1Name;
static const char *m_team2Name;
static int m_team1NameLength;

static void resetResultTimer();
static void showResultSprites();
static void hideResultSprites();
static int getScorerListOffsetY();
static void positionTeamNameSprites(int scorerListOffsetY);
static void positionResultGridSprites();
static void positionResultDigitSprites(int scorerListOffsetY);
static void positionDashSprite(int scorerListOffsetY);
static void positionResultSprites();
static void positionScorerNameSprites(int scorerListOffsetY);
static void updateScorersText(const Sprite& scorer, const TeamGame& team, int teamNum, ScorerInfo& scorerInfo, int currentLine);

void resetResult(const char *team1Name, const char *team2Name)
{
    swos.team1NameSprite.pictureIndex = kTeam1NameSprite;
    swos.team2NameSprite.pictureIndex = kTeam2NameSprite;

    for (int i = 0; i < kMaxScorersForDisplay; i++) {
        m_team1Scorers[i].shirtNum = 0;
        m_team2Scorers[i].shirtNum = 0;
        m_team1ScorerLines[i][0] = '\0';
        m_team2ScorerLines[i][0] = '\0';
    }

    hideResultSprites();

    m_team1Name = team1Name;
    m_team2Name = team2Name;
    m_team1NameLength = getStringPixelLength(team1Name, true);
}

void showAndPositionResult()
{
    if (swos.resultTimer < 0) {
        hideResult();
    } else if (swos.resultTimer > 0) {
        if (swos.resultTimer == kEndOfHalfResult || swos.resultTimer == kGameBreakResult || swos.resultTimer == kMaxResultTicks) {
            resetResultTimer();
            showResultSprites();
        }

        swos.resultTimer -= swos.timerDifference;
        if (swos.resultTimer <= 0) {
            if (swos.gameState == GameState::kResultOnHalftime || swos.gameState == GameState::kResultAfterTheGame)
                swos.statsTimer = kMaxResultTicks;
            hideResult();
        } else {
            int scorerListOffsetY = getScorerListOffsetY();
            positionTeamNameSprites(scorerListOffsetY);
            positionResultGridSprites();
            positionResultDigitSprites(scorerListOffsetY);
            positionDashSprite(scorerListOffsetY);
            positionScorerNameSprites(scorerListOffsetY);
            positionResultSprites();
        }
    }
}

void hideResult()
{
    swos.resultTimer = 0;

    if (swos.team1NameSprite.visible)
        hideResultSprites();
}

void registerScorer(const Sprite& scorer, int teamNum, GoalType goalType)
{
    assert(swos.topTeamPtr && swos.bottomTeamPtr);

    ScorerInfo *scorers;
    TeamGame *scoringTeam, *concedingTeam;

    if (teamNum == 1) {
        scorers = m_team1Scorers.data();
        scoringTeam = swos.topTeamPtr;
        concedingTeam = swos.bottomTeamPtr;
    } else {
        scorers = m_team2Scorers.data();
        scoringTeam = swos.bottomTeamPtr;
        concedingTeam = swos.topTeamPtr;
    }

    auto& player = scoringTeam->players[scorer.playerOrdinal - 1];
    int shirtNum = player.shirtNumber;

    if (goalType == GoalType::kOwnGoal) {
        std::swap(scoringTeam, concedingTeam);
        concedingTeam->numOwnGoals++;
        shirtNum += 1'000;  // this is to differentiate a player that scored own goal from a guy with the same
                            // number in the opposite team (since own-goals appear under the opponent's goals)
    } else {
        player.goalsScored++;
    }

    int currentSlot = 0;
    int currentLine = 0;

    while (currentSlot < kMaxScorersForDisplay && currentLine < kMaxScorersForDisplay) {
        auto& scorerInfo = scorers[currentSlot];
        if (!scorerInfo.shirtNum || scorerInfo.shirtNum == shirtNum) {
            if (!scorerInfo.shirtNum) {
                scorerInfo.numGoals = 0;
                scorerInfo.numLines = 1;
            }
            scorerInfo.shirtNum = shirtNum;
            if (scorerInfo.numGoals != kMaxGoalsPerScorer) {
                auto& goal = scorerInfo.goals[scorerInfo.numGoals++];
                const auto& gameTime = gameTimeAsBcd();
                goal.update(goalType, gameTime);
                updateScorersText(scorer, *scoringTeam, teamNum, scorerInfo, currentLine);
            }
            break;
        } else {
            currentSlot++;
            currentLine += scorerInfo.numLines;
        }
    }
}

void drawScorers(int teamNum)
{
    const auto& sprite = teamNum == 1 ? swos.team1Scorer1Sprite : swos.team2Scorer1Sprite;
    const auto& lines = teamNum == 1 ? m_team1ScorerLines : m_team2ScorerLines;

    assert(!sprite.x.fraction() && !sprite.y.fraction() && !sprite.z.fraction());

    int y = sprite.y.whole() - sprite.z.whole();
    auto drawTextRoutine = teamNum == 2 ? drawText : drawTextRightAligned;

    for (size_t i = 0; i < lines.size() && lines[i][0]; i++) {
        int x = sprite.x.whole();
        drawTextRoutine(x, y, lines[i].data(), -1, kWhiteText, false);
        y += kScorerLineHeight;
    }
}

void drawTeamName(int teamNum)
{
    const auto& sprite = teamNum == 1 ? swos.team1NameSprite : swos.team2NameSprite;
    const auto teamName = teamNum == 1 ? m_team1Name : m_team2Name;

    assert(!sprite.x.fraction() && !sprite.y.fraction() && !sprite.z.fraction());

    drawText(sprite.x.whole(), sprite.y.whole() - sprite.z.whole(), teamName, -1, kWhiteText, true);
}

static void resetResultTimer()
{
    assert(swos.resultTimer == kEndOfHalfResult || swos.resultTimer == kGameBreakResult || swos.resultTimer == kMaxResultTicks);

    switch (swos.resultTimer) {
    case kEndOfHalfResult:
        swos.resultTimer = kResultAtHalfTimeLength;
        break;
    case kGameBreakResult:
        swos.resultTimer = kResultAtGameBreakLength;
        break;
    case kMaxResultTicks:
        swos.resultTimer = kResultTickClamped;
        break;
    }
}

static void showResultSprites()
{
    if (swos.gameState == GameState::kResultOnHalftime)
        swos.resultOnHalftimeSprite.show();
    else if (swos.gameState == GameState::kResultAfterTheGame)
        swos.resultAfterTheGameSprite.show();

    if (swos.dontShowScorers || swos.secondLeg) {
        swos.team1TotalGoalsDigit2Sprite.show();
        if (swos.team1TotalGoals / 10)
            swos.team1TotalGoalsDigit1Sprite.show();

        swos.team2TotalGoalsDigit2Sprite.show();
        if (swos.team2TotalGoals / 10)
            swos.team2TotalGoalsDigit1Sprite.show();
    }

    swos.gridUpperRowLeftSprite.show();
    swos.gridUpperRowRightSprite.show();
    swos.gridMiddleRowLeftSprite.show();
    swos.gridMiddleRowRightSprite.show();
    swos.gridBottomRowLeftSprite.show();
    swos.gridBottomRowRightSprite.show();
    swos.team1NameSprite.show();
    swos.team2NameSprite.show();
    swos.team1GoalsDigit2Sprite.show();

    if (swos.team1GoalsDigit1)
        swos.team1GoalsDigit1Sprite.show();

    swos.team2GoalsDigit2Sprite.show();

    if (swos.team2GoalsDigit1)
        swos.team2GoalsDigit1Sprite.show();

    swos.team1NameSprite.show();
    swos.dashSprite.show();

    if (!swos.dontShowScorers) {
        static const std::pair<const ScorerLines&, Sprite *> kTeamScorerData[] = {
            { std::ref(m_team1ScorerLines), &swos.team1Scorer1Sprite, },
            { std::ref(m_team2ScorerLines), &swos.team2Scorer1Sprite, },
        };
        for (const auto& scorerData : kTeamScorerData) {
            for (size_t i = 0; i < kMaxScorersForDisplay; i++) {
                if (scorerData.first[i][0]) {
                    scorerData.second->show();
                    break;
                }
            }
        }
    }

    initDisplaySprites();
}

static void hideResultSprites()
{
    static Sprite * const kResultSprites[] = {
        &swos.resultOnHalftimeSprite, &swos.resultAfterTheGameSprite,
        &swos.team1NameSprite, &swos.team2NameSprite,
        &swos.dashSprite,
        &swos.gridUpperRowLeftSprite, &swos.gridUpperRowRightSprite,
        &swos.gridMiddleRowLeftSprite, &swos.gridMiddleRowRightSprite,
        &swos.gridBottomRowLeftSprite, &swos.gridBottomRowRightSprite,
        &swos.team1GoalsDigit2Sprite, &swos.team1GoalsDigit1Sprite,
        &swos.team1TotalGoalsDigit2Sprite, &swos.team1TotalGoalsDigit1Sprite,
        &swos.team2GoalsDigit2Sprite, &swos.team2GoalsDigit1Sprite,
        &swos.team2TotalGoalsDigit2Sprite, &swos.team2TotalGoalsDigit1Sprite,
    };

    for (auto sprite : kResultSprites)
        sprite->hide();

    swos.team1Scorer1Sprite.hide();
    swos.team2Scorer1Sprite.hide();

    initDisplaySprites();
}

static int getScorerListOffsetY()
{
    int scorerListOffsetY = 0;

    // move team name up 7 pixels for each scorer line
    for (size_t i = 0; i < m_team1ScorerLines.size() && (m_team1ScorerLines[i][0] || m_team2ScorerLines[i][0]); i++)
        scorerListOffsetY -= kScorerLineHeight;

    // force at least 14 pixels between team name and scorer list
    return std::min(scorerListOffsetY, -2 * kScorerLineHeight) - 64;
}

static void positionTeamNameSprites(int scorerListOffsetY)
{
    swos.team1NameSprite.x = kLeftMargin - m_team1NameLength;
    swos.team1NameSprite.y = kTeamNameY + scorerListOffsetY + kResultTextZ;
    swos.team1NameSprite.z = kResultTextZ;

    swos.team2NameSprite.x = kRightMargin;
    swos.team2NameSprite.y = swos.team1NameSprite.y;
    swos.team2NameSprite.z = kResultTextZ;
}

static void positionResultGridSprites()
{
    auto cameraX = getCameraX();

    auto y = swos.team1NameSprite.y - 1'015 + getCameraY(); // team 1 name y + 9,000 - 15

    swos.gridUpperRowLeftSprite.x = cameraX;
    swos.gridUpperRowLeftSprite.y = y;
    swos.gridUpperRowLeftSprite.z = kResultGridZ;

    swos.gridUpperRowRightSprite.x = cameraX + kRightGridX;
    swos.gridUpperRowRightSprite.y = y;
    swos.gridUpperRowRightSprite.z = kResultGridZ;

    y += 32;

    swos.gridMiddleRowLeftSprite.x = cameraX;
    swos.gridMiddleRowLeftSprite.y = y;
    swos.gridMiddleRowLeftSprite.z = kResultGridZ;

    swos.gridMiddleRowRightSprite.x = cameraX + kRightGridX;
    swos.gridMiddleRowRightSprite.y = y;
    swos.gridMiddleRowRightSprite.z = kResultGridZ;

    y += 32;

    swos.gridBottomRowLeftSprite.x = cameraX;
    swos.gridBottomRowLeftSprite.y = y;
    swos.gridBottomRowLeftSprite.z = kResultGridZ;

    swos.gridBottomRowRightSprite.x = cameraX + kRightGridX;
    swos.gridBottomRowRightSprite.y = y;
    swos.gridBottomRowRightSprite.z = kResultGridZ;
}

static void positionResultDigitSprites(int scorerListOffsetY)
{
    auto cameraX = getCameraX();
    auto cameraY = getCameraY();

    swos.team1GoalsDigit2Sprite.x = cameraX + kBigLeftResultDigitX;
    swos.team1GoalsDigit2Sprite.y = cameraY + kBigLeftResultDigitY + scorerListOffsetY + kResultTextZ;
    swos.team1GoalsDigit2Sprite.z = kResultTextZ;
    swos.team1GoalsDigit2Sprite.pictureIndex = kBigZeroSprite + swos.team1GoalsDigit2;

    if (swos.team1GoalsDigit1Sprite.visible) {
        swos.team1GoalsDigit1Sprite.x = swos.team1GoalsDigit2Sprite.x - kResultBigDigitWidth;
        swos.team1GoalsDigit1Sprite.y = swos.team1GoalsDigit2Sprite.y;
        swos.team1GoalsDigit1Sprite.z = swos.team1GoalsDigit2Sprite.z;
        swos.team1GoalsDigit1Sprite.pictureIndex = kBigZeroSprite + swos.team1GoalsDigit1;
    }

    if (swos.team1TotalGoalsDigit2Sprite.visible) {
        auto team1GoalsDigit1 = swos.team1TotalGoals / 10;
        auto team1GoalsDigit2 = swos.team1TotalGoals % 10;

        swos.team1TotalGoalsDigit2Sprite.x = cameraX + kSmallLeftResultDigitX;
        if (team1GoalsDigit2 == 1)
            swos.team1TotalGoalsDigit2Sprite.x += kResultDigit1Offset;
        swos.team1TotalGoalsDigit2Sprite.y = cameraY + kFirstLineBelowResultY + scorerListOffsetY + kSmallDigitZ;
        swos.team1TotalGoalsDigit2Sprite.z = kSmallDigitZ;
        swos.team1TotalGoalsDigit2Sprite.pictureIndex = kSmallZeroSprite + team1GoalsDigit2;

        if (swos.team1TotalGoalsDigit1Sprite.visible) {
            swos.team1TotalGoalsDigit1Sprite.x = swos.team1TotalGoalsDigit2Sprite.x - kResultSmallDigitWidth;
            if (team1GoalsDigit1 == 1)
                swos.team1TotalGoalsDigit1Sprite.x += kResultDigit1Offset;

            swos.team1TotalGoalsDigit1Sprite.y = swos.team1TotalGoalsDigit2Sprite.y - 1;
            swos.team1TotalGoalsDigit1Sprite.z = swos.team1TotalGoalsDigit2Sprite.z - 1;
            swos.team1TotalGoalsDigit1Sprite.pictureIndex = kSmallZeroSprite + team1GoalsDigit1;
        }
    }

    swos.team2GoalsDigit2Sprite.x = cameraX + kBigRightResultSecondDigitX;
    swos.team2GoalsDigit2Sprite.y = cameraY + kBigLeftResultDigitY + scorerListOffsetY + kResultTextZ;
    swos.team2GoalsDigit2Sprite.z = kResultTextZ;
    swos.team2GoalsDigit2Sprite.pictureIndex = swos.team2GoalsDigit2 + kBigZeroSprite;

    if (swos.team2GoalsDigit1Sprite.visible) {
        swos.team2GoalsDigit1Sprite.x = swos.team2GoalsDigit2Sprite.x;
        swos.team2GoalsDigit2Sprite.x += kResultBigDigitWidth;
        swos.team2GoalsDigit1Sprite.y = swos.team2GoalsDigit2Sprite.y;
        swos.team2GoalsDigit1Sprite.z = swos.team2GoalsDigit2Sprite.z;
        swos.team2GoalsDigit1Sprite.pictureIndex = swos.team2GoalsDigit1 + kBigZeroSprite;
    }

    if (swos.team2TotalGoalsDigit2Sprite.visible) {
        auto team2GoalsDigit1 = swos.team2TotalGoals / 10;
        auto team2GoalsDigit2 = swos.team2TotalGoals % 10;

        swos.team2TotalGoalsDigit2Sprite.x = cameraX + kSmallRightResultDigitX;
        swos.team2TotalGoalsDigit2Sprite.y = cameraY + kFirstLineBelowResultY + scorerListOffsetY + kSmallDigitZ;
        swos.team2TotalGoalsDigit2Sprite.z = kSmallDigitZ;
        swos.team2TotalGoalsDigit2Sprite.pictureIndex = kSmallZeroSprite + team2GoalsDigit2;

        if (swos.team2TotalGoalsDigit1Sprite.visible) {
            swos.team2TotalGoalsDigit1Sprite.x = swos.team2TotalGoalsDigit2Sprite.x;
            swos.team2TotalGoalsDigit2Sprite.x += kResultSmallDigitWidth;
            if (team2GoalsDigit1 == 1)
                swos.team2TotalGoalsDigit1Sprite.x -= kResultDigit1Offset;
            swos.team2TotalGoalsDigit1Sprite.y = swos.team2TotalGoalsDigit2Sprite.y - 1;
            swos.team2TotalGoalsDigit1Sprite.z = swos.team2TotalGoalsDigit2Sprite.z - 1;
            swos.team2TotalGoalsDigit1Sprite.pictureIndex = kSmallZeroSprite + team2GoalsDigit1;
        }
    }
}

static void positionDashSprite(int scorerListOffsetY)
{
    swos.dashSprite.x = getCameraX() + kDashX;
    swos.dashSprite.y = getCameraY() + kDashY + scorerListOffsetY + kResultTextZ;
    swos.dashSprite.z = kResultTextZ;
}

void positionResultSprites()
{
    swos.resultOnHalftimeSprite.x = getCameraX() + kResultX;
    swos.resultOnHalftimeSprite.y = swos.gridUpperRowLeftSprite.y + 990;
    swos.resultOnHalftimeSprite.z = kResultTextZ;

    swos.resultAfterTheGameSprite.x = swos.resultOnHalftimeSprite.x;
    swos.resultAfterTheGameSprite.y = swos.resultOnHalftimeSprite.y;
    swos.resultAfterTheGameSprite.z = swos.resultOnHalftimeSprite.z;
}

static void positionScorerNameSprites(int scorerListOffsetY)
{
    // don't add camera coordinates, we'll have special handling routine for this
    swos.team1Scorer1Sprite.x = kLeftMargin;
    swos.team1Scorer1Sprite.y = kFirstLineBelowResultY + scorerListOffsetY + kResultTextZ;
    swos.team1Scorer1Sprite.z = kResultTextZ;

    swos.team2Scorer1Sprite.x = kRightMargin;
    swos.team2Scorer1Sprite.y = kFirstLineBelowResultY + scorerListOffsetY + kResultTextZ;
    swos.team2Scorer1Sprite.z = kResultTextZ;
}

static const char *getPlayerName(int playerOrdinal, const TeamGame& team)
{
    auto name = team.players[playerOrdinal].shortName;
    int offset = name[0] && name[1] && name[1] == '.' && name[2] == ' ' ? 3 : 0;
    return name + offset;
}

static bool shiftLinesDown(ScorerLines& lines, int lineToFree)
{
    if (lineToFree >= kMaxScorersForDisplay)
        return false;

    memmove(lines[lineToFree + 1].data(), lines[lineToFree].data(), (lines.size() - lineToFree - 1) * lines[0].size());

    return true;
}

static void updateScorersText(const Sprite& scorer, const TeamGame& team, int teamNum, ScorerInfo& scorerInfo, int currentLine)
{
    assert(currentLine < kMaxScorersForDisplay);

    auto name = getPlayerName(scorer.playerOrdinal - 1, team);
    auto& lines = teamNum == 1 ? m_team1ScorerLines : m_team2ScorerLines;
    auto line = lines[currentLine].data();

    int i = 0;
    while (line[i++] = *name++)
        ;

    line[i - 1] = '\t'; // preserve 5 pixels spacing as in the original
    line[i] = '\0';

    int width = getStringPixelLength(line);
    line += i;

    int currentScorerLine = 0;
    char goalMinuteBuf[16];

    for (int i = 0; i < scorerInfo.numGoals; i++) {
        const auto& goal = scorerInfo.goals[i];
        int minuteBufLen = goal.storeTime(goalMinuteBuf);

        switch (goal.type) {
        case GoalType::kPenalty:
            strcpy(&goalMinuteBuf[minuteBufLen], "(PEN)");
            minuteBufLen += 5;
            break;
        case GoalType::kOwnGoal:
            strcpy(&goalMinuteBuf[minuteBufLen], "(OG)");
            minuteBufLen += 4;
            break;
        }

        if (i != scorerInfo.numGoals - 1)
            goalMinuteBuf[minuteBufLen++] = ',';

        goalMinuteBuf[minuteBufLen] = '\0';
        int goalMinuteLen = getStringPixelLength(goalMinuteBuf);

        if (width + goalMinuteLen <= kMaxScorerLineWidth) {
            width += goalMinuteLen;
        } else {
            if (++currentLine >= kMaxScorersForDisplay)
                break;

            if (++currentScorerLine >= scorerInfo.numLines) {
                if (!shiftLinesDown(lines, currentLine))
                    break;
                scorerInfo.numLines++;
            }

            *line = '\0';
            line = lines[currentLine].data();
            width = goalMinuteLen;
        }

        assert(line + minuteBufLen < lines[currentLine].data() + kCharactersPerLine);

        memcpy(line, goalMinuteBuf, minuteBufLen);
        line += minuteBufLen;
    }

    *line = '\0';
}
