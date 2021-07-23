#include "windowManager.h"
#include "darkRectangle.h"
#include "text.h"

constexpr int kLeftColumn = 80;
constexpr int kMiddleColumn = 160;
constexpr int kRightColumn = 240;

constexpr int kTeamNamesY = 43;
constexpr int kGoalsY = 61;
constexpr int kGoalAttempsY = 97;

constexpr int kLineSpacing = 18;

static void drawDarkRectangles();
static void drawText();

static void drawStatistics()
{
    drawDarkRectangles();
    drawText();
}

void SWOS::DrawStatistics()
{
    drawStatistics();
}

static void drawDarkRectangles()
{
    static const std::array<SDL_FRect, 10> kDarkRects = {{
        { 80, 19, 160, 16 }, { 16, 39, 288, 16 }, { 16, 57, 288, 16 }, { 16, 75, 288, 16 },
        { 16, 93, 288, 16 }, { 16, 111, 288, 16 }, { 16, 129, 288, 16 }, { 16, 147, 288, 16 },
        { 16, 165, 288, 16 }, { 16, 183, 288, 16 },
    }};

    auto xScale = getXScale();
    auto yScale = getYScale();

    auto rects = kDarkRects;

    for (auto& rect : rects) {
        rect.x = rect.x * xScale;
        rect.y = rect.y * yScale;
        rect.w = rect.w * xScale;
        rect.h = rect.h * yScale;
    }

    darkenRectangles(rects.data(), rects.size());
}

static std::pair<const TeamStatsData *, const TeamStatsData *> getTeamStatsPointers()
{
    auto leftTeam = &swos.topTeamData;
    auto rightTeam = &swos.bottomTeamData;

    if (leftTeam->inGameTeamPtr.asAligned() != &swos.topTeamIngame)
        std::swap(leftTeam, rightTeam);

    return { leftTeam->teamStatsPtr.asAligned(), rightTeam->teamStatsPtr.asAligned() };
}

static void drawStatsText(int x, int y, const char *text)
{
    drawTextCentered(x, y, text, -1, kWhiteText, true);
}

static void drawStatsNumber(int x, int y, int value, bool appendPercent = false)
{
    char buf[32];
    SDL_itoa(value, buf, 10);
    if (appendPercent)
        strcat(buf, "%");
    drawStatsText(x, y, buf);
}

static void drawTeamNames()
{
    int leftTeamX = kLeftColumn;
    int rightTeamX = kRightColumn;
    if (swos.g_trainingGame) {
        leftTeamX += 5;
        rightTeamX += 5;
    }

    drawStatsText(leftTeamX, kTeamNamesY, swos.topTeamIngame.teamName);
    drawStatsText(rightTeamX, kTeamNamesY, swos.bottomTeamIngame.teamName);
}

static void drawPossession(const TeamStatsData *leftStats, const TeamStatsData *rightStats)
{
    drawStatsText(160, 79, "POSSESSION");

    if (int totalPossession = leftStats->ballPossession + rightStats->ballPossession) {
        int leftTeamPossessionPercentage = 100 * leftStats->ballPossession / totalPossession;
        drawStatsNumber(80, 79, leftTeamPossessionPercentage, true);
        drawStatsNumber(240, 79, 100 - leftTeamPossessionPercentage, true);
    } else {
        drawStatsText(80, 79, "-");
        drawStatsText(240, 79, "-");
    }
}

static void drawText()
{
    drawStatsText(kMiddleColumn, 23, "M A T C H   S T A T S");

    drawTeamNames();

    drawStatsText(160, kGoalsY, "GOALS");
    drawStatsNumber(80, kGoalsY, swos.statsTeam1Goals);
    drawStatsNumber(240, kGoalsY, swos.statsTeam2Goals);

    const TeamStatsData *leftStats, *rightStats;
    std::tie(leftStats, rightStats) = getTeamStatsPointers();

    drawPossession(leftStats, rightStats);

    struct {
        const char *description;
        int leftValue;
        int rightValue;
    } const kStatsDisplayInfo[] = {
        "GOAL ATTEMPTS", leftStats->goalAttempts, rightStats->goalAttempts,
        "ON TARGET", leftStats->onTarget, rightStats->onTarget,
        "CORNERS WON", leftStats->cornersWon, rightStats->cornersWon,
        "FOULS CONCEEDED", leftStats->foulsConceded, rightStats->foulsConceded,
        "BOOKINGS", leftStats->bookings, rightStats->bookings,
        "SENDINGS OFF", leftStats->sendingsOff, rightStats->sendingsOff,
    };

    int y = kGoalAttempsY;
    for (const auto& statsInfo : kStatsDisplayInfo) {
        drawStatsText(kMiddleColumn, y, statsInfo.description);
        drawStatsNumber(kLeftColumn, y, statsInfo.leftValue);
        drawStatsNumber(kRightColumn, y, statsInfo.rightValue);
        y += kLineSpacing;
    }
}
