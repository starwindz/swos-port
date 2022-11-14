// Routines for capturing input and essential game state, in order to create files which
// could later be used to verify that any game recreation faithfully follows the original.

#include "gamedata.h"
#include "bfile.h"
#include <initializer_list>

constexpr int kVersionMajor = 1;
constexpr int kVersionMinor = 2;

#define kDir "recdata"
constexpr int kDirSize = sizeof(kDir) - 1;

constexpr int kNumSprites = 78;

enum TeamControls {
    kTeamNotSelected = 0,
    kComputerTeam = 1,
    kPlayerCoach = 2,
    kCoach = 3,
};

enum ControlFlags {
    kUp = 1,
    kDown = 2,
    kLeft = 4,
    kRight = 8,
    kFire = 32,
    kBench = 64,
    kEscape = 128,
};

constexpr int kFrameNumSprites = 33;

#pragma pack(push, 1)
struct Frame {
    dword frameNo;
    int16_t player1Controls;
    int16_t player2Controls;
    TeamGeneralInfo topTeam;
    char topTeamPlayers[11];
    TeamGeneralInfo bottomTeam;
    char bottomTeamPlayers[11];
    TeamGame topTeamGame;
    TeamGame bottomTeamGame;
    TeamStatsData topTeamStats;
    TeamStatsData bottomTeamStats;
    FixedPoint cameraX;
    FixedPoint cameraY;
    int8_t cameraXVelocity;
    int8_t cameraYVelocity;
    dword gameTime;
    byte team1Goals;
    byte team2Goals;
    byte gameState;
    byte gameStatePl;
    Sprite sprites[kFrameNumSprites];
};
#pragma pack(pop)

// keep synced with code in tests
static const char kIncludedSprites[kNumSprites] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0,
};
// +1 for the unused sprite
static const char kSpriteLocationToIndex[kNumSprites + 1] = {
    68, 67, 70, 71, 72, 73, 74, 75, 76, 77, 66, -1, 30, 31, 32, 39, 40, 44, 42, 45,
    43, 48, 46, 49, 47, 41, 50, 52, 54, 56, 58, 60, 62, 64, 51, 53, 55, 57, 59, 61,
    63, 65, 28, 69, 26, 27, 29, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
    18, 19, 20, 21, 22, 23, 24, 25, 1, 0, 33, 34, 35, 36, 37, 38, 2, 3
};
static char m_spriteIndexMap[kNumSprites];

static BFile m_file;
static char m_dataBuffer[64 * 1024];

static uint32_t m_frameNo;
static byte m_randomValue;

static Frame m_frame;
static bool m_firstFrame;

static bool m_gameRunning;
static bool m_firstPl1Controls;
static bool m_firstPl2Controls;

static char m_replayFilenameToRead[64];
static bool m_replaying;
static bool m_fastForward;

static byte m_topTeamCoachNo;
static byte m_topTeamPlayerNo;
static byte m_bottomTeamCoachNo;
static byte m_bottomTeamPlayerNo;

static int8_t m_extraTime;
static int8_t m_penaltyShootout;

static int8_t m_topTeamMarkedPlayer = -1;
static int8_t m_bottomTeamMarkedPlayer = -1;

static byte m_mainKeysCheck1stByte;
static bool m_aborted;
static bool m_trainingGame;

bool Player1StatusProcHook();
bool Player1StatusProcHook();
static void initSpriteIndexMap();
static void startRecordingGame();
static void readOrRecordFrame();
static void flipNoFrameAction();
Frame getCurrentStateFrame();
static void fillControlFlags(Frame& f);
static void fillTeam(const TeamGeneralInfo& team, TeamGeneralInfo& outTeam, char *spriteIndices);
static void fillSprites(Frame& f);
static void fillSprite(Sprite& dstSprite, const Sprite& src);
static void setShotChanceTable(TeamGeneralInfo& team);
static Sprite *spriteToIndex(const Sprite *sprite);
static bool pointerValid(void *ptr);
static void hookGameLoopInit();
static void hookPlayTrainingGame();
static void startGameReplay();
static void enqueueEscapeKey();
static void resetSwosVariables();
static void resetSprites();
static void fixedRand();
static void generateFixedRandomValue(const char *filename);
static void writeHeader();
static void readHeader();
static void verifyFrame();
static void applyControlFlags();
static void generateFilename(char *buffer);
static void safeWrite(const void *buf, int size);
static void safeRead(void *buf, int size);

// One-time only initialization at start up.
void initGameData()
{
    if (!CreateDirectory(kDir)) {
        WriteToLog("Failed to create game data directory.");
        exit(1);
    }

    initSpriteIndexMap();

    PatchByte(Rand, 0, 0xe9);
    PatchCall(Rand, 1, fixedRand);
    PatchByte(Rand2, 0, 0xe9);
    PatchCall(Rand2, 1, fixedRand);

    PatchCall(StartMainGameLoop, 0x39, hookGameLoopInit);

    PatchByte(Flip, 0, 0xe8);
    PatchCall(Flip, 1, readOrRecordFrame);
    PatchDword(Flip, 5, 0x0fffc883);    // or ax, -1 to force the jz below not to be taken

    PatchCall(GameOver, 0x3a, flipNoFrameAction);

    PatchCall(SelectTraining, 1, hookPlayTrainingGame);

    *(byte *)ReplayExitMenuAfterFriendly = 0xc3;    // disable replay menu after the game
    PatchByte(QuitToDosPromptLoop, 9, 0xc3);        // as well as quit to DOS menu

    // skip stoppageTimer increment from the timer tick interrupt handler
    *(word *)((char *)TimerProc + 0x24) = 0x05eb;

    // g_currentTick is used in calculation which player wins the ball, so we will replace it with stoppageTimer
    memcpy((char *)CalculateIfPlayerWinsBall + 0x30d, (char *)GameLoop + 0x592, 4);

    m_mainKeysCheck1stByte = *(byte *)MainKeysCheck;
    *(byte *)MainKeysCheck = 0xc3;

    spinBigS = 0;

    if (m_replaying)
        PatchDword(MenuProc, 1, startGameReplay);
}

void stopRecordingGame()
{
    CloseBFileUnmanaged(&m_file);
    m_trainingGame = false;
}

void setReplayFile(const char *filename, int length, bool fastForward)
{
    auto p = strcpy(m_replayFilenameToRead, kDir);
    *p++ = '\\';
    int bufferSize = sizeof(m_replayFilenameToRead) - kDirSize - 1;
    if (length < bufferSize)
        bufferSize = length;
    *strncpy(p, filename, bufferSize) = '\0';
    WriteToLog("Replay file to read set: %s", m_replayFilenameToRead);
    m_replaying = m_replayFilenameToRead[0];
    m_fastForward = fastForward;
}

// Called by ASM code.
extern "C" bool Player1StatusProcAllowed()
{
    if (!m_gameRunning)
        return true;

    bool result = m_firstPl1Controls;
    m_firstPl1Controls = false;
    return result;
}

extern "C" bool Player2StatusProcAllowed()
{
    if (!m_gameRunning)
        return true;

    bool result = m_firstPl2Controls;
    m_firstPl2Controls = false;
    return result;
}

static void initSpriteIndexMap()
{
    int offset = 0;
    for (int i = 0; i < kNumSprites; i++)
        m_spriteIndexMap[i] = kIncludedSprites[i] ? offset++ : -1;
}

static void startRecordingGame()
{
    if (m_replaying)
        return;

    char path[kDirSize + 1 + 12 + 1];
    memcpy(path, kDir "\\", kDirSize + 1);

    auto filename = path + kDirSize + 1;

    generateFilename(filename);

    if (!CreateBFileUnmanaged(&m_file, m_dataBuffer, sizeof(m_dataBuffer), ATTR_NONE, path)) {
        WriteToLog("Failed to create file %s", path);
        exit(1);
    }

    generateFixedRandomValue(filename);
    writeHeader();

    m_frameNo = 0;

    resetSwosVariables();
}

static void readOrRecordFrame()
{
    m_firstPl1Controls = true;
    m_firstPl2Controls = true;

    if (m_replaying) {
        static bool teamsLogged;
        if (!teamsLogged) {
            WriteToLog("Top team: %s, bottom team: %s", leftTeamData.inGameTeamPtr->teamName,
                rightTeamData.inGameTeamPtr->teamName);
            teamsLogged = true;
        }

        if (m_firstFrame) {
            extraTimeState = m_extraTime;
            penaltiesState = m_penaltyShootout;
            leftTeamIngame.markedPlayer = m_topTeamMarkedPlayer;
            rightTeamIngame.markedPlayer = m_bottomTeamMarkedPlayer;
            m_firstFrame = false;
        } else {
            // we're verifying the last frame, right before we start the next one
            verifyFrame();
            stoppageTimer += !paused;
        }

        // when reading don't skip first Flip() call since it puts us in ideal position (just before 1st frame)
        safeRead(&m_frame, sizeof(m_frame));
        m_frameNo = m_frame.frameNo;
        // apply the inputs
        applyControlFlags();
    } else {
        // skip 1st frame since Flip() gets called before entire game loop has finished (in order to fade in)
        if (m_firstFrame) {
            m_firstFrame = false;
            return;
        }

        auto frame = getCurrentStateFrame();
        safeWrite(&frame, sizeof(frame));

        m_frameNo += !paused;
        stoppageTimer += !paused;   // simulate timer tick
    }
}

static void flipNoFrameAction()
{
    calla((char *)Flip + 0xe);
}

Frame getCurrentStateFrame()
{
    Frame f;

    f.frameNo = m_frameNo;
    fillControlFlags(f);

    fillTeam(leftTeamData, f.topTeam, f.topTeamPlayers);
    fillTeam(rightTeamData, f.bottomTeam, f.bottomTeamPlayers);

    f.topTeamGame = leftTeamIngame;
    f.bottomTeamGame = rightTeamIngame;
    f.topTeamStats = team1StatsData;
    f.bottomTeamStats = team2StatsData;

    memcpy(&f.cameraX, &cameraXFraction, 8);

    assert(cameraXVelocity >= -40 && cameraXVelocity <= 40 && cameraYVelocity >= -40 && cameraYVelocity <= 40);
    f.cameraXVelocity = cameraXVelocity;
    f.cameraYVelocity = cameraYVelocity;

    f.gameTime = gameTime;

    f.team1Goals = statsTeam1Goals;
    f.team2Goals = statsTeam2Goals;

    f.gameState = gameState;
    f.gameStatePl = gameStatePl;

    fillSprites(f);

    return f;
}

static void fillControlFlags(Frame& f)
{
    // we can't use keyboard control flags directly since they're set asynchronously from a keyboard interrupt
    // handler; instead, recreate it from PlayerXStatusProc() output variables
    f.player1Controls = 0;
    if (pl1Left)
        f.player1Controls |= kLeft;
    if (pl1Right)
        f.player1Controls |= kRight;
    if (pl1Up)
        f.player1Controls |= kUp;
    if (pl1Down)
        f.player1Controls |= kDown;
    if (pl1Fire)
        f.player1Controls |= kFire;
    if (pl1SecondaryFire)
        f.player1Controls |= kBench;

    f.player2Controls = 0;
    if (pl2Left)
        f.player2Controls |= kLeft;
    if (pl2Right)
        f.player2Controls |= kRight;
    if (pl2Up)
        f.player2Controls |= kUp;
    if (pl2Down)
        f.player2Controls |= kDown;
    if (pl2Fire)
        f.player2Controls |= kFire;
    if (pl2SecondaryFire)
        f.player2Controls |= kBench;

    if (lastKey && !paused && !showingStats && playGame && convertedKey == 1) {
        f.player1Controls |= kEscape;
        f.player2Controls |= kEscape;
        enqueueEscapeKey();
    }
}

static void fillTeam(const TeamGeneralInfo& team, TeamGeneralInfo& outTeam, char *spriteIndices)
{
    outTeam = team;

    assert(team.opponentsTeam == &leftTeamData || team.opponentsTeam == &rightTeamData);
    outTeam.opponentsTeam = (TeamGeneralInfo *)(team.opponentsTeam == &leftTeamData ? 0 : 1);

    assert(team.inGameTeamPtr == &leftTeamIngame || team.inGameTeamPtr == &rightTeamIngame);
    outTeam.inGameTeamPtr = (TeamGame *)(team.inGameTeamPtr == &leftTeamIngame ? 0 : 1);

    assert(team.teamStatsPtr == &team1StatsData || team.teamStatsPtr == &team2StatsData);
    outTeam.teamStatsPtr = (TeamStatsData *)(team.teamStatsPtr == &team1StatsData ? 0 : 1);

    assert(team.players == &team1SpritesTable || team.players == &team2SpritesTable);
    outTeam.players = (decltype(team.players))(team.players == &team1SpritesTable ? 0 : 1);

    setShotChanceTable(outTeam);

    for (int i = 0; i < 11; i++)
        spriteIndices[i] = (int)spriteToIndex((*team.players)[i]);

    outTeam.controlledPlayerSprite = spriteToIndex(outTeam.controlledPlayerSprite);
    outTeam.passToPlayerPtr = spriteToIndex(outTeam.passToPlayerPtr);
    outTeam.lastHeadingPlayer = spriteToIndex(outTeam.lastHeadingPlayer);
    outTeam.passingKickingPlayer = spriteToIndex(outTeam.passingKickingPlayer);
}

static void fillSprites(Frame& f)
{
    int index = 0;
    for (int i = 0; i < kNumSprites; i++) {
        if (kIncludedSprites[i]) {
            assert(index < kFrameNumSprites);
            auto sprite = allSpritesArray[i];
            fillSprite(f.sprites[index++], *sprite);
        }
    }
}

static void fillSprite(Sprite& dst, const Sprite& src)
{
    assert(!pointerValid(src.animTablePtr) ||
        (src.animTablePtr >= animTablesStart && src.animTablePtr < (void *)&goalBasePtr));
    assert(!pointerValid(src.frameIndicesTable) || src.frameIndicesTable == ballStaticFrameIndices ||
        src.frameIndicesTable == ballMovingFrameIndices ||
        (src.frameIndicesTable >= frameIndicesTablesStart && src.frameIndicesTable < animTablesStart) ||
        (src.frameIndicesTable >= strongHeaderUpFrames && src.frameIndicesTable < (void *)&choosingPreset));

    dst = src;

    if (!pointerValid(src.animTablePtr))
        dst.animTablePtr = (void *)-1;
    else
        *(char **)&dst.animTablePtr -= (int)animTablesStart;

    if (!pointerValid(src.frameIndicesTable)) {
        dst.frameIndicesTable = (void *)-1;
    } else if (src.frameIndicesTable == ballMovingFrameIndices) {
        dst.frameIndicesTable = (void *)-2;
    } else if (src.frameIndicesTable == ballStaticFrameIndices) {
        dst.frameIndicesTable = (void *)-3;
    } else if (src.frameIndicesTable >= frameIndicesTablesStart &&
        src.frameIndicesTable < (void *)&goalBasePtr) {
        *(char **)&dst.frameIndicesTable -= (int)frameIndicesTablesStart;
    } else {
        int offset = (int)dst.frameIndicesTable;
        offset = (offset - (int)strongHeaderUpFrames) | 0x80000000;
        dst.frameIndicesTable = (void *)offset;
    }
}

static void setShotChanceTable(TeamGeneralInfo& team)
{
    assert(!team.shotChanceTable || team.shotChanceTable == teamPlOfs24Table ||
        (team.shotChanceTable >= goalieSkill7Table && team.shotChanceTable <= goalieSkill0Table));

    int offset = 0;
    if (!team.shotChanceTable) {
        offset = -1;
    } else if (team.shotChanceTable != teamPlOfs24Table) {
        assert((team.shotChanceTable - goalieSkill7Table) % 30 == 0);
        offset = -2;
        for (size_t i = 0; i < sizeofarray(goalieSkillTables); i++) {
            if (goalieSkillTables[i] == team.shotChanceTable) {
                offset = i + 1;
                break;
            }
        }
    }

    team.shotChanceTable = (int16_t *)offset;
}

static Sprite *spriteToIndex(const Sprite *sprite)
{
    assert(!sprite || (sprite >= &big_S_Sprite && sprite <= &goal2BottomSprite));

    int offset = -1;

    if (sprite) {
        offset = sprite - &big_S_Sprite;
        assert(offset >= 0 && offset <= kNumSprites);
        int allSpritesIndex = kSpriteLocationToIndex[offset];
        assert(allSpritesIndex >= 0 && allSpritesIndex < kNumSprites);
        offset = m_spriteIndexMap[allSpritesIndex];
    }

    return (Sprite *)offset;
}

static bool pointerValid(void *ptr)
{
    return ptr && ptr != (void *)-1;
}

static void hookGameLoopInit()
{
    m_firstFrame = true;
    m_gameRunning = true;
    startRecordingGame();
    calla(GameLoop);
    stopRecordingGame();
    m_gameRunning = false;
}

static void hookPlayTrainingGame()
{
    if (!m_replaying)
        m_trainingGame = true;
    calla(PlayTrainingGame);
}

static void startGameReplay()
{
    static bool initialized;
    if (!initialized) {
        initialized = true;

        PatchByte(SetControlWord, 0, 0xc3);
        PatchByte(Joy1SetStatus, 0, 0xc3);
        PatchByte(Joy2SetStatus, 0, 0xc3);

        PatchWord(Player1StatusProc, 8, 0x15eb);
        PatchWord(Player2StatusProc, 8, 0x15eb);

        *(byte *)MainKeysCheck = 0xc3;

        readHeader();

        if (m_trainingGame) {
            calla(PlayTrainingGame);
        } else {
            g_numSelectedTeams = 2;
            A1 = (dword)g_selectedTeams;
            A2 = (dword)(g_selectedTeams + 1);
            calla(InitAndPlayGame);
        }
    } else {
        if (m_aborted) {
            WriteToLog("Recorded game was aborted, quitting");
            exit(0);
        }
        static int timesCalled;
        int plNum = 0;
        if (!timesCalled) {
            if (g_selectedTeams[0].teamStatus != kComputerTeam)
                plNum = g_selectedTeams[0].teamStatus == kCoach ? m_topTeamCoachNo : m_topTeamPlayerNo;
            else if (g_selectedTeams[1].teamStatus != kComputerTeam)
                plNum = g_selectedTeams[1].teamStatus == kCoach ? m_bottomTeamCoachNo : m_bottomTeamPlayerNo;
        } else {
            assert(g_selectedTeams[0].teamStatus != kComputerTeam &&
                g_selectedTeams[1].teamStatus != kComputerTeam);
            plNum = g_selectedTeams[1].teamStatus == kCoach ? m_bottomTeamCoachNo : m_bottomTeamPlayerNo;
        }
        timesCalled++;
        playerNumThatStarted = plNum;
    }

    calla(ReadTimerDelta);
    calla(DrawMenu);

    calla(SetExitMenuFlag);
}

static void enqueueEscapeKey()
{
    *(byte *)MainKeysCheck = m_mainKeysCheck1stByte;
    keyCount = 1;
    keyBuffer[0] = 1;
}

// Resets pretty much the same stuff a multiplayer game would.
static void resetSwosVariables()
{
    teamSwitchCounter = 0;
    gameStoppedTimer = 0;
    goalCounter = 0;
    stateGoal = 0;
    pl1BenchTimeoutCounter = benchCounter2 = 0;

    // not sure if we need these, but just in case
    longFireFlag = longFireTime = longFireCounter = 0;

    // we _definitely_ need this UGH $%*@(#$*! found it out the hard way... and it's only used ONCE
    frameCount = 0;
    cameraXFraction = cameraYFraction = 0;  // just in case... they don't seem to like clearing fractions
    cameraXVelocity = cameraYVelocity = 0;

    // must reset these cause 2nd player controls might have something in them if pl2 on keyboard
    pl1LastFired = pl2LastFired = 0;
    g_joy1Status = g_joy2Status = 0;
    pl1ShortFireCounter = pl2ShortFireCounter = 0;
    pl1Left = 0;
    pl1Right = 0;
    pl1Up = 0;
    pl1Down = 0;
    pl1Fire = 0;
    pl1SecondaryFire = 0;
    pl2Left = 0;
    pl2Right = 0;
    pl2Up = 0;
    pl2Down = 0;
    pl2Fire = 0;
    pl2SecondaryFire = 0;

    EGA_graphics = 0;

    // gotta reset teams as there are some timers in there
    memset((char *)&leftTeamData + 24, 0, sizeof(leftTeamData) - 24);
    memset((char *)&rightTeamData + 24, 0, sizeof(leftTeamData) - 24);

    paused = 0;
    stoppageTimer = 0;

    replaySelected = 0;

    resetSprites();
}

// Also nicked from multiplayer game.
static void resetSprites()
{
    const Sprite * const kSentinel = (Sprite *)-1;

    for (auto currentSprite = allSpritesArray; *currentSprite != kSentinel; currentSprite++) {
        auto s = *currentSprite;
        if (s->shirtNumber) {
            // fraction part needs to be initialized to zero, or it will have a leftover value from previous game
            s->x.setFraction(0);
            s->y.setFraction(0);
            s->ballDistance = 0;
            s->deltaX = s->deltaY = s->deltaZ = 0;
            s->playerDirection = 0;
            s->isMoving = 0;
            // not sure what these are, but they gotta be synchronized too
            s->unk009 = 0;
            s->unk010 = 0;
        }

        s->delayedFrameTimer = -1;
        s->frameIndex = -1;
        s->cycleFramesTimer = 1;
        s->direction = 0;
        s->startingDirection = 0;
        s->fullDirection = 0;
    }

    // last, but not least, fix the ball itself
    ballSprite->deltaX = ballSprite->deltaY = ballSprite->deltaZ = 0;
    *(word *)&ballSprite->x = 0;
    *(word *)&ballSprite->y = 0;
    *(word *)&ballSprite->z = 0;
}

static void fixedRand()
{
    D0 = m_randomValue;
}

static void generateFixedRandomValue(const char *filename)
{
    m_randomValue = 0;
    int i = 0;

    while (*filename)
        m_randomValue += 17 * (*filename++ ^ i++);

    WriteToLog("Fixed random value: %hhu/%hhx", m_randomValue);
}

// header format:
//  version major (byte)
//  version minor (byte)
//  fixed random value (byte)
//  pitch type or season (int8_t)
//  pitch type (int8_t)
//  game length (int8_t)
//  auto replays (int8_t)
//  all player teams equal (int8_t)
//  top team (file)
//  bottom team
//  topTeamCoachNo (byte)
//  topTeamPlayerNo (byte)
//  bottomTeamCoachNo (byte)
//  bottomTeamPlayerNo (byte)
//  extraTimes (byte)
//  penaltyShootout (byte)
//  top team marked player (int8_t)
//  bottom team marked player (int8_t)
//  all 6 user tactics
//  selected input controls (word)
//  initial frame # (dword)
//  v1.2 only:
//    size of career file for training game that follows (dword)
//    season (int8_t)
//    minSubstitutes (int8_t)
//    maxSubstitutes (int8_t)
//
static void writeHeader()
{
    uint8_t version[] = { kVersionMajor, kVersionMinor };
    safeWrite(version, sizeof(version));
    safeWrite(&m_randomValue, sizeof(uint8_t));
    if (m_trainingGame) {
        gamePitchTypeOrSeason = 0;
        gamePitchType = -1;
        gameSeason = 8;
        minSubstitutes = 2;
        maxSubstitutes = 2;
    }
    safeWrite(&gamePitchTypeOrSeason, 1);
    safeWrite(&gamePitchType, 1);
    safeWrite(&gameLength, 1);
    safeWrite(&autoReplays, 1);
    safeWrite(&allPlayerTeamsEqual, 1);
    if (m_trainingGame) {
        safeWrite(&careerTeam, sizeof(TeamFile));
        safeWrite(&currentTeam, sizeof(TeamFile));
    } else {
        safeWrite(gameTeam1, sizeof(TeamFile));
        safeWrite(gameTeam2, sizeof(TeamFile));
    }
    safeWrite(&leftTeamCoachNo, 1);
    safeWrite(&leftTeamPlayerNo, 1);
    safeWrite(&rightTeamCoachNo, 1);
    safeWrite(&rightTeamPlayerNo, 1);
    safeWrite(&extraTimeState, 1);
    safeWrite(&penaltiesState, 1);
    safeWrite(&leftTeamIngame.markedPlayer, 1);
    safeWrite(&rightTeamIngame.markedPlayer, 1);
    safeWrite(USER_A, 6 * sizeof(Tactics));
    safeWrite(&g_joyKbdWord, sizeof(word));
    safeWrite(&m_frameNo, sizeof(uint32_t));
    int careerSize = 0;
    if (m_trainingGame)
        careerSize = (int)&g_numSelectedTeams - (int)careerFileBuffer +
            g_numSelectedTeams * sizeof(TeamFile) + 2;
    safeWrite(&careerSize, 4);
    safeWrite(&gameSeason, 1);
    safeWrite(&minSubstitutes, 1);
    safeWrite(&maxSubstitutes, 1);
    if (m_trainingGame)
        safeWrite(careerFileBuffer, careerSize);
}

static void readHeader()
{
    assert(m_replaying);

    WriteToLog("Opening replay file %s", m_replayFilenameToRead);
    if (!OpenBFileUnmanaged(&m_file, m_dataBuffer, sizeof(m_dataBuffer), F_READ_ONLY, m_replayFilenameToRead)) {
        WriteToLog("Failed to open replay file %s", m_replayFilenameToRead);
        exit(1);
    }

    WriteToLog("Reading replay file header...");

    uint8_t version[2];
    safeRead(version, 2);
    WriteToLog("Reading replay file version %d.%d", version[0], version[1]);
    if (version[0] != kVersionMajor) {
        WriteToLog("Incompatible file, major version %d, can only read up to %d", version[0], kVersionMajor);
        exit(1);
    }

    safeRead(&m_randomValue, 1);
    WriteToLog("Random value: %d", m_randomValue);

    for (auto var : { &gamePitchTypeOrSeason, &gamePitchType, &gameLength, &autoReplays, &allPlayerTeamsEqual }) {
        int8_t val;
        safeRead(&val, 1);
        *var = val;
    }
    WriteToLog("gamePitchTypeOrSeason: %hd", gamePitchTypeOrSeason);
    WriteToLog("gamePitchType: %hd", gamePitchType);
    WriteToLog("gameLength: %hd", gameLength);
    WriteToLog("autoReplays: %hd", autoReplays);
    WriteToLog("allPlayerTeamsEqual: %hd", allPlayerTeamsEqual);

    safeRead(g_selectedTeams, 2 * sizeof(TeamFile));

    safeRead(&m_topTeamCoachNo, 1);
    safeRead(&m_topTeamPlayerNo, 1);
    safeRead(&m_bottomTeamCoachNo, 1);
    safeRead(&m_bottomTeamPlayerNo, 1);

    WriteToLog("top team player: %d", m_topTeamPlayerNo);
    WriteToLog("top team coach: %d", m_topTeamCoachNo);
    WriteToLog("bottom team player: %d", m_bottomTeamPlayerNo);
    WriteToLog("bottom team coach: %d", m_bottomTeamCoachNo);

    safeRead(&m_extraTime, 1);
    safeRead(&m_penaltyShootout, 1);

    safeRead(&m_topTeamMarkedPlayer, 1);
    safeRead(&m_bottomTeamMarkedPlayer, 1);

    safeRead(USER_A, 6 * sizeof(Tactics));
    safeRead(&g_joyKbdWord, sizeof(word));
    safeRead(&m_frameNo, sizeof(uint32_t));

    if (version[0] > 1 || version[1] >= 2) {
        int careerSize;
        safeRead(&careerSize, 4);
        m_trainingGame = !!careerSize;

        for (auto var : { &gameSeason, &minSubstitutes, &maxSubstitutes }) {
            int8_t val;
            safeRead(&val, 1);
            *var = val;
        }

        TeamFile teamBuf[2];
        memcpy(teamBuf, g_selectedTeams, 2 * sizeof(TeamFile));

        WriteToLog("Loading career file %d bytes long", careerSize);
        safeRead(careerFileBuffer, careerSize);

        memcpy(&careerTeam, teamBuf, sizeof(TeamFile));
        memcpy(&currentTeam, teamBuf + 1, sizeof(TeamFile));
    }

    WriteToLog("Initial frame: %d", m_frameNo);
    WriteToLog("Input controls: %d", g_joyKbdWord);
    WriteToLog("Is training game: %d", m_trainingGame);

    resetSwosVariables();

    if (m_fastForward) {
        PatchWord(Flip, 0x21, 0x12eb);
        PatchByte(WaitRetrace, 0, 0xc3);
    }
}

static void verifyFrame()
{
    auto frame = getCurrentStateFrame();

    auto p1 = (byte *)&m_frame;
    auto p2 = (byte *)&frame;

    size_t i;
    for (i = 0; i < sizeof(frame); i++)
        if (p1[i] != p2[i])
            break;

    if (i != sizeof(frame)) {
        WriteToLog("Frame mismatch at frame %d offset %d", m_frameNo, i);
        HexDumpToLog(&m_frame, sizeof(m_frame), "Expected Frame");
        HexDumpToLog(&frame, sizeof(frame), "Actual Frame");
        exit(1);
    }
}

static void applyControlFlags()
{
    g_joy1Status = m_frame.player1Controls & ~kEscape;
    g_joy2Status = m_frame.player2Controls & ~kEscape;
    if ((m_frame.player1Controls & kEscape) || (m_frame.player2Controls & kEscape)) {
        lastKey = convertedKey = 1;
        m_aborted = true;
        enqueueEscapeKey();
    }
}

static void generateFilename(char *buffer)
{
    word year;
    byte month, day, hour, minute, second, hundred;
    GetDosDate(&year, &month, &day);
    GetDosTime(&hour, &minute, &second, &hundred);

    sprintf(buffer, "%02d%02d%02d%02d.rgd", month % 13, hour % 24, minute % 60, second % 60);

    WriteToLog("Generated game data filename: %s", buffer);

    assert(strlen(buffer) == 12);
}

static void safeWrite(const void *buf, int size)
{
    if (WriteBFile(&m_file, buf, size) != size) {
        WriteToLog("Failed to write game data file, buffer = %#x, size = %d", buf, size);
        exit(1);
    }
}

static void safeRead(void *buf, int size)
{
    int readSize = ReadBFile(&m_file, buf, size);
    if (readSize != size) {
        WriteToLog("Failed to read game data file at position %d, buffer = %#x, size = %d, read bytes = %hd",
            GetBFilePos(&m_file), buf, size, readSize);
        exit(1);
    }
}
