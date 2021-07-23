#include "gameSprites.h"
#include "sprites.h"
#include "renderSprites.h"
#include "colorizeSprites.h"
#include "playerNameDisplay.h"
#include "result.h"
#include "replays.h"
#include "camera.h"
#include "gameTime.h"
#include "darkRectangle.h"

static Sprite * const kAllSprites[] = {
    &swos.ballShadowSprite,
    &swos.ballSprite,
    &swos.goal1TopSprite,
    &swos.goal2BottomSprite,
    &swos.goalie1Sprite,
    &swos.team1Player2Sprite,
    &swos.team1Player3Sprite,
    &swos.team1Player4Sprite,
    &swos.team1Player5Sprite,
    &swos.team1Player6Sprite,
    &swos.team1Player7Sprite,
    &swos.team1Player8Sprite,
    &swos.team1Player9Sprite,
    &swos.team1Player10Sprite,
    &swos.team1Player11Sprite,
    &swos.goalie2Sprite,
    &swos.team2Player2Sprite,
    &swos.team2Player3Sprite,
    &swos.team2Player4Sprite,
    &swos.team2Player5Sprite,
    &swos.team2Player6Sprite,
    &swos.team2Player7Sprite,
    &swos.team2Player8Sprite,
    &swos.team2Player9Sprite,
    &swos.team2Player10Sprite,
    &swos.team2Player11Sprite,
    &swos.team1CurPlayerNumSprite,
    &swos.team2CurPlayerNumSprite,
    &swos.playerMarkSprite,
    &swos.bookedPlayerSprite,
    &swos.currentTimeSprite,
    &swos.resultOnHalftimeSprite,
    &swos.resultAfterTheGameSprite,
    &swos.gridUpperRowLeftSprite,
    &swos.gridUpperRowRightSprite,
    &swos.gridMiddleRowLeftSprite,
    &swos.gridMiddleRowRightSprite,
    &swos.gridBottomRowLeftSprite,
    &swos.gridBottomRowRightSprite,
    &swos.team1NameSprite,
    &swos.team2NameSprite,
    &swos.dashSprite,
    &swos.team1GoalsDigit1Sprite,
    &swos.team2GoalsDigit1Sprite,
    &swos.team1GoalsDigit2Sprite,
    &swos.team2GoalsDigit2Sprite,
    &swos.team1TotalGoalsDigit1Sprite,
    &swos.team2TotalGoalsDigit1Sprite,
    &swos.team1TotalGoalsDigit2Sprite,
    &swos.team2TotalGoalsDigit2Sprite,
    &swos.team1Scorer1Sprite,
    &swos.team2Scorer1Sprite,
    &swos.refereeSprite,
    &swos.cornerFlagSprite,
    &swos.big_S_Sprite,
    &swos.currentPlayerNameSprite,
};

static std::array<Sprite *, std::size(kAllSprites)> m_sortedSprites;
static int m_numSpritesToRender;

static const TeamGame *m_topTeam;
static const TeamGame *m_bottomTeam;

static void sortDisplaySprites();
static std::pair<float, float> transformCamera(const Sprite& sprite, float cameraX, float cameraY);
static bool handleSpecialSprites(const Sprite *sprite, int& i, int screenWidth, int screenHeight, float cameraX, float cameraY);

void initGameSprites(const TeamGame *topTeam, const TeamGame *bottomTeam)
{
    m_topTeam = topTeam;
    m_bottomTeam = bottomTeam;

    initializePlayerSpriteFrameIndices();
}

// Prepares display sprites for rendering. Sorts them by y-axis.
void initDisplaySprites()
{
    m_numSpritesToRender = 0;

    for (auto sprite : kAllSprites)
        if (sprite->visible)
            m_sortedSprites[m_numSpritesToRender++] = sprite;

    sortDisplaySprites();
}

void initializePlayerSpriteFrameIndices()
{
    constexpr int kNumGoalkeeperSprites = kTeam1ReserveGoalkeeperSpriteStart - kTeam1MainGoalkeeperSpriteStart;
    constexpr int kNumPlayerSprites = kTeam1GingerPlayerSpriteStart - kTeam1WhitePlayerSpriteStart;

    const auto kTeamData = {
        std::make_tuple(swos.team1SpritesTable, m_topTeam, true),
        std::make_tuple(swos.team2SpritesTable, m_bottomTeam, false)
    };

    for (const auto& teamData : kTeamData) {
        auto team = std::get<1>(teamData);
        auto spriteTable = std::get<0>(teamData);
        bool topTeam = std::get<2>(teamData);

        auto goalie = getGoalkeeperIndexFromFace(topTeam, team[0].players[0].face);
        assert(goalie == 0 || goalie == 1);
        spriteTable[0]->frameOffset = goalie * kNumGoalkeeperSprites;

        for (size_t i = 1; i < std::size(swos.team1SpritesTable); i++) {
            auto& player = team->players[i];
            auto& sprite = spriteTable[i];

            assert(player.face <= 3);
            sprite->frameOffset = player.face * kNumPlayerSprites;
        }
    }
}

#ifdef DEBUG
static void verifySprites()
{
    for (const auto& sprite : kAllSprites) {
        auto assertIn = [sprite](int start, int end, bool allowEmpty = false) {
            if (!allowEmpty || sprite->pictureIndex != -1)
                assert(sprite->pictureIndex >= start && sprite->pictureIndex <= end);
        };

        if (sprite == &swos.ballShadowSprite)
            assert(sprite->pictureIndex == kBallShadowSprite || sprite->pictureIndex == -1);
        else if (sprite == &swos.ballSprite)
            assertIn(kBallSprite1, kBallSprite4, true);
        else if (sprite == &swos.goal1TopSprite)
            assert(sprite->pictureIndex == kTopGoalSprite);
        else if (sprite == &swos.goal2BottomSprite)
            assert(sprite->pictureIndex == kBottomGoalSprite);
        else if (sprite == &swos.goalie1Sprite)
            assertIn(kTeam1MainGoalkeeperSpriteStart, kTeam1ReserveGoalkeeperSpriteEnd);
        else if (sprite == &swos.goalie2Sprite)
            assertIn(kTeam2MainGoalkeeperSpriteStart, kTeam2ReserveGoalkeeperSpriteEnd);
        else if (sprite == &swos.team1Player2Sprite || sprite == &swos.team1Player3Sprite ||
            sprite == &swos.team1Player4Sprite || sprite == &swos.team1Player5Sprite ||
            sprite == &swos.team1Player6Sprite || sprite == &swos.team1Player7Sprite ||
            sprite == &swos.team1Player8Sprite || sprite == &swos.team1Player9Sprite ||
            sprite == &swos.team1Player10Sprite || sprite == &swos.team1Player11Sprite)
            assertIn(kTeam1WhitePlayerSpriteStart, kTeam1BlackPlayerSpriteEnd);
        else if (sprite == &swos.team2Player2Sprite || sprite == &swos.team2Player3Sprite ||
            sprite == &swos.team2Player4Sprite || sprite == &swos.team2Player5Sprite ||
            sprite == &swos.team2Player6Sprite || sprite == &swos.team2Player7Sprite ||
            sprite == &swos.team2Player8Sprite || sprite == &swos.team2Player9Sprite ||
            sprite == &swos.team2Player10Sprite || sprite == &swos.team2Player11Sprite)
            assertIn(kTeam2WhitePlayerSpriteStart, kTeam2BlackPlayerSpriteEnd);
        else if (sprite == &swos.team1CurPlayerNumSprite || sprite == &swos.team2CurPlayerNumSprite)
            assertIn(kSmallDigit1, kSmallDigit16, true);
        else if (sprite == &swos.playerMarkSprite)
            assert(sprite->pictureIndex == kPlayerMarkSprite || sprite->pictureIndex == -1);
        else if (sprite == &swos.bookedPlayerSprite)
            assert(sprite->pictureIndex >= kSmallDigit1 && sprite->pictureIndex <= kSmallDigit16 ||
                sprite->pictureIndex == kRedCardSprite || sprite->pictureIndex == kYellowCardSprite ||
                sprite->pictureIndex == -1);
        else if (sprite == &swos.currentTimeSprite)
            assert(sprite->pictureIndex == kTimeSprite118Mins || sprite->pictureIndex == -1);
        else if (sprite == &swos.resultOnHalftimeSprite)
            assert(sprite->pictureIndex == kHalfTimeSprite);
        else if (sprite == &swos.resultAfterTheGameSprite)
            assert(sprite->pictureIndex == kFullTimeSprite);
        else if (sprite == &swos.gridUpperRowLeftSprite || sprite == &swos.gridUpperRowRightSprite ||
            sprite == &swos.gridMiddleRowLeftSprite || sprite == &swos.gridMiddleRowRightSprite ||
            sprite == &swos.gridBottomRowLeftSprite || sprite == &swos.gridBottomRowRightSprite)
            assert(sprite->pictureIndex == kSquareGridForResultSprite);
        else if (sprite == &swos.team1NameSprite)
            assert(sprite->pictureIndex == kTeam1NameSprite);
        else if (sprite == &swos.team2NameSprite)
            assert(sprite->pictureIndex == kTeam2NameSprite);
        else if (sprite == &swos.dashSprite)
            assert(sprite->pictureIndex == kBigDashSprite);
        else if (sprite == &swos.team1GoalsDigit1Sprite || sprite == &swos.team2GoalsDigit1Sprite ||
            sprite == &swos.team1GoalsDigit2Sprite || sprite == &swos.team2GoalsDigit2Sprite)
            assertIn(kBigZeroSprite, kBigNineSprite, true);
        else if (sprite == &swos.team1TotalGoalsDigit1Sprite || sprite == &swos.team2TotalGoalsDigit1Sprite ||
            sprite == &swos.team1TotalGoalsDigit2Sprite || sprite == &swos.team2TotalGoalsDigit2Sprite)
            assertIn(kSmallZeroSprite, kSmallNineSprite, true);
        else if (sprite == &swos.team1Scorer1Sprite)
            assertIn(kTeam1Scorer1NameSprite, kTeam1Scorer8NameSprite);
        else if (sprite == &swos.team2Scorer1Sprite)
            assertIn(kTeam2Scorer1NameSprite, kTeam2Scorer8NameSprite);
        else if (sprite == &swos.refereeSprite)
            assertIn(kRefereeSpriteStart, kRefereeSpriteEnd, true);
        else if (sprite == &swos.cornerFlagSprite)
            assertIn(kCornerFlagSpriteStart, kCornerFlagSpriteEnd);
        else if (sprite == &swos.big_S_Sprite)
            assertIn(kBigSSpriteStart, kBigSSpriteEnd, true);
        else if (sprite == &swos.currentPlayerNameSprite)
            assertIn(kTeam1PlayerNamesStartSprite, kTeam2PlayerNamesEndSprite, true);
    }
}
#endif

// The place where game sprites get drawn to the screen.
void drawSprites()
{
#ifdef DEBUG
    verifySprites();
#endif

    sortDisplaySprites();

    int screenWidth, screenHeight;
    std::tie(screenWidth, screenHeight) = getWindowSize();

    auto cameraX = getCameraX();
    auto cameraY = getCameraY();

    for (int i = 0; i < m_numSpritesToRender; i++) {
        auto sprite = m_sortedSprites[i];

        if (handleSpecialSprites(sprite, i, screenWidth, screenHeight, cameraX, cameraY))
            continue;

        float x, y;
        std::tie(x, y) = transformCamera(*sprite, cameraX, cameraY);

        const auto& spriteImage = drawSprite(sprite->pictureIndex, x, y, screenWidth, screenHeight);

        // since screen can potentially be huge don't reject any sprites for highlights, just dump them all there
        if (sprite != &swos.big_S_Sprite && sprite->teamNumber)
            saveCoordinatesForHighlights(sprite->pictureIndex, std::lround(x), std::lround(y));

        int iX = std::lround(x);
        int iY = std::lround(y);

        // check if this is correct
        if (iX < 336 && iY < 200 && iX > -spriteImage.width && iY > -spriteImage.height)
            sprite->beenDrawn = 1;
        else
            sprite->beenDrawn = 0;
    }
}

void SWOS::InitDisplaySprites()
{
    initDisplaySprites();
}

static void sortDisplaySprites()
{
    std::sort(m_sortedSprites.begin(), m_sortedSprites.begin() + m_numSpritesToRender, [](const auto& spr1, const auto& spr2) {
        return spr1->y < spr2->y;
    });
}

static void growRectangle(SDL_FRect& rect, float x, float y, int screenWidth, int screenHeight)
{
    constexpr int kGridWidth = 168;
    constexpr int kGridHeight = 32;

    auto xScale = getXScale();
    auto yScale = getYScale();

    x *= xScale;
    y *= yScale;

    auto width = kGridWidth * xScale;
    auto height = kGridHeight * yScale;

    // use w/h as endX/endY coordinates
    if (x < rect.x)
        rect.x = x;
    if (y < rect.y)
        rect.y = y;
    if (x + width > rect.w)
        rect.w = x + width;
    if (y + height > rect.h)
        rect.h = y + height;
}

static std::pair<float, float> transformCamera(const Sprite& sprite, float cameraX, float cameraY)
{
    float x = sprite.x;
    float y = sprite.y;
    float z = sprite.z;

    x -= cameraX;
    y -= cameraY + z;

    return { x, y };
}

// SWOS draws darkened rectangles using special sprite which has a fixed width and height. That's why this
// sprite is drawn many times to cover larger areas. The problem is that these areas overlap in the game
// which causes problems since we're drawing it in blended mode (while they simply set high bit in the pixel
// to take advantage of specially constructed palette). The solution is to combine the area into a single
// rect first, and then draw this rect. It has to be done immediately and not at the end since it would
// disrupt z-order.
static void drawDarkRectangles(int& i, int screenWidth, int screenHeight, float cameraX, float cameraY)
{
    assert(m_sortedSprites[i]->pictureIndex == kSquareGridForResultSprite);

    constexpr float kBigValue = 999'999;
    SDL_FRect darkRectangle{ kBigValue, kBigValue, -kBigValue, -kBigValue };

    for (; i < m_numSpritesToRender; i++) {
        auto sprite = m_sortedSprites[i];
        if (sprite->pictureIndex != kSquareGridForResultSprite)
            break;

        float x, y;
        std::tie(x, y) = transformCamera(*sprite, cameraX, cameraY);

        growRectangle(darkRectangle, x, y, screenWidth, screenHeight);
    }

    drawDarkRectangle(darkRectangle, screenWidth, screenHeight);
}

static bool handleSpecialSprites(const Sprite *sprite, int& i, int screenWidth, int screenHeight, float cameraX, float cameraY)
{
    assert(sprite && (sprite->pictureIndex == -1 || sprite->visible));

    if (sprite->pictureIndex == -1) {
        return true;
    } else if (sprite == &swos.currentTimeSprite) {
        drawGameTime(*sprite);
        return true;
    } else if (sprite->pictureIndex == kSquareGridForResultSprite) {
        drawDarkRectangles(i, screenWidth, screenHeight, cameraX, cameraY);
        return true;
    } else if (sprite->pictureIndex >= kTeam1PlayerNamesStartSprite && sprite->pictureIndex <= kTeam2PlayerNamesEndSprite) {
        drawPlayerName(sprite->pictureIndex);
        return true;
    } else if (sprite->pictureIndex == kTeam1Scorer1NameSprite || sprite->pictureIndex == kTeam2Scorer1NameSprite) {
        drawScorers(sprite->pictureIndex == kTeam1Scorer1NameSprite ? 1 : 2);
        return true;
    } else if (sprite->pictureIndex == kTeam1NameSprite || sprite->pictureIndex == kTeam2NameSprite) {
        drawTeamName(sprite->pictureIndex == kTeam1NameSprite ? 1 : 2);
        return true;
    }

    return false;
}
