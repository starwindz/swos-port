#include "stadiumMenu.h"
#include "menuBackground.h"
#include "gameSprites.h"
#include "sprites.h"
#include "controls.h"
#include "drawMenu.h"
#include "versusMenu.h"
#include "stadium.mnu.h"

struct SavedSprite {
    SavedSprite(int index, const PackedSprite& sprite) : sprite(sprite), index(index) {}
    PackedSprite sprite;
    int index;
};

std::vector<SavedSprite> m_savedSpriteDimensions;

static const TeamGame *m_team1;
static const TeamGame *m_team2;
static int m_maxSubstitutes;

static void setupTeamNames();
static void setupPlayerSprites();
static void setupPlayerNames();
static void shrinkSpriteFrame(int index, int widthMul, int widthDiv, int heightMul, int heightDiv);
static void restorePlayerSpriteFrames();
static bool spriteAlreadyShrunk(int index);

using namespace StadiumMenu;

void showStadiumMenu(const TeamGame *team1, const TeamGame *team2, int maxSubstitutes)
{
    assert(team1 && team2);

    m_team1 = team1;
    m_team2 = team2;
    m_maxSubstitutes = maxSubstitutes;

    showMenu(stadiumMenu);
}

static void stadiumMenuOnInit()
{
    constexpr char kBackgroundImageBasename[] = "stad";

    setMenuBackgroundImage(kBackgroundImageBasename);

    setupTeamNames();
    setupPlayerSprites();
    setupPlayerNames();

    drawMenu(false);
    restorePlayerSpriteFrames();

    fadeInAndOut([]() {
        SDL_Delay(300);
        processControlEvents();
    });

    SetExitMenuFlag();
}

static void setupTeamNames()
{
    setTeamNameAndColor(leftTeamNameEntry, *m_team1, swos.leftTeamCoachNo, swos.leftTeamPlayerNo);
    setTeamNameAndColor(rightTeamNameEntry, *m_team2, swos.rightTeamCoachNo, swos.rightTeamPlayerNo);
}

static void setupPlayerSprites()
{
    constexpr int kPlayerFrameOffset = 77;
    constexpr int kGoalKeeperFrameOffset = 3;

    m_savedSpriteDimensions.clear();

    static const auto kTeamData = {
        std::make_tuple(&leftTeamPlayerSpritesStartEntry, m_team1, true),
        std::make_tuple(&rightTeamPlayerSpritesStartEntry, m_team2, false),
    };

    for (const auto& teamData : kTeamData) {
        auto entry = std::get<0>(teamData);
        auto team = std::get<1>(teamData);
        auto topTeam = std::get<2>(teamData);

        int spriteIndex = topTeam ? kTeam1MainGoalkeeperSpriteStart : kTeam2MainGoalkeeperSpriteStart;
        spriteIndex += kGoalKeeperFrameOffset + getGoalkeeperSpriteOffset(topTeam, team->players[0].face2);

        shrinkSpriteFrame(spriteIndex, 3, 4, 4, 5);
        const auto& sprite = getSprite(spriteIndex);

        entry->setSprite(spriteIndex);
        entry->x += static_cast<int16_t>(std::lround(sprite.centerXF));
        entry++->y += static_cast<int16_t>(std::lround(sprite.centerYF));

        for (int i = 1; i < kNumPlayerSprites; i++) {
            const auto& player = team->players[i];

            int spriteIndex = topTeam ? kTeam1WhitePlayerSpriteStart : kTeam2WhitePlayerSpriteStart;
            spriteIndex += kPlayerFrameOffset + getPlayerSpriteOffsetFromFace(player.face2);

            shrinkSpriteFrame(spriteIndex, 9, 16, 2, 3);
            const auto& sprite = getSprite(spriteIndex);

            entry->setSprite(spriteIndex);
            entry->x += static_cast<int16_t>(std::lround(sprite.centerXF));
            entry++->y += static_cast<int16_t>(std::lround(sprite.centerYF));
        }
    }
}

static void setupPlayerNames()
{
    static_assert(StadiumMenu::kNumPlayerNames <= sizeofarray(m_team1->players), "Too many names!");

    static const auto kTeamData = {
        std::make_pair(&lefTeamPlayerNamesStartEntry, m_team1),
        std::make_pair(&rightTeamPlayerNamesStartEntry, m_team2),
    };

    for (const auto& teamData : kTeamData) {
        auto entry = teamData.first;
        auto team = teamData.second;
        for (int i = 0; i < StadiumMenu::kNumPlayerNames; i++) {
            const auto& player = team->players[i];
            entry++->copyString(player.fullName);
        }

        // hide extraneous reserves (based on gameMaxSubstitues)
        auto lastPlayer = entry - 1;
        for (int i = 5 - m_maxSubstitutes; i > 0; i--)
            lastPlayer--->hide();
    }
}

static void shrinkSpriteFrame(int index, int widthMul, int widthDiv, int heightMul, int heightDiv)
{
    if (!spriteAlreadyShrunk(index)) {
        auto sprite = getSprite(index);
        m_savedSpriteDimensions.emplace_back(index, sprite);
        sprite.widthF = sprite.widthF * widthMul / widthDiv;
        sprite.heightF = sprite.heightF * heightMul / heightDiv;
        sprite.width = sprite.width * widthMul / widthDiv;
        sprite.height = sprite.height * heightMul / heightDiv;
        sprite.frame.w = sprite.frame.w * widthMul / widthDiv;
        sprite.frame.h = sprite.frame.h * heightMul / heightDiv;
        setSprite(index, sprite);
    }
}

static void restorePlayerSpriteFrames()
{
    for (const auto& sprite : m_savedSpriteDimensions)
        setSprite(sprite.index, sprite.sprite);
}

bool spriteAlreadyShrunk(int index)
{
    auto it = std::find_if(m_savedSpriteDimensions.begin(), m_savedSpriteDimensions.end(), [index](const auto& savedSprite) {
        return savedSprite.index == index;
    });
    return it != m_savedSpriteDimensions.end();
}
