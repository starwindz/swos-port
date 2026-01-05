#include "gameLoop.h"
#include "game.h"
#include "ball.h"
#include "team.h"
#include "render.h"
#include "windowManager.h"
#include "timer.h"
#include "gameControls.h"
#include "spinningLogo.h"
#include "playerNameDisplay.h"
#include "sprites.h"
#include "gameSprites.h"
#include "updateSprite.h"
#include "updatePlayers.h"
#include "gameTime.h"
#include "bench.h"
#include "drawBench.h"
#include "result.h"
#include "stats.h"
#include "pitch.h"
#include "pitchConstants.h"
#include "referee.h"
#include "menus.h"
#include "menuBackground.h"
#include "camera.h"
#include "controls.h"
#include "keyBuffer.h"
#include "audio.h"
#include "music.h"
#include "comments.h"
#include "sfx.h"
#include "chants.h"
#include "options.h"
#include "replays.h"
#include "stadiumMenu.h"
#include "replayExitMenu.h"
#include "util.h"
#include "FixedPoint.h"

constexpr FixedPoint kGameEndCameraX = 176;
constexpr FixedPoint kGameEndCameraY = 80;

static bool m_fadeAndSaveReplay;
static bool m_fadeAndInstantReplay;
static bool m_fadeAndReplayHighlights;

static bool m_doFadeIn;

static bool m_playingMatch;

static int m_penaltiesInterval = 110;
static int m_initalKickInterval = 825;
static int m_goalCameraInterval = 55;
static int m_allowPlayerControlCameraInterval = 550;

#ifdef SWOS_TEST
static std::function<void()> m_gameLoopStartHook = []() {};
static std::function<void()> m_gameLoopEndHook = []() {};
#endif

static void initGameLoop();
static void drawFrame(bool recordingEnabled);
static void gameFadeOut();
static void gameFadeIn();
static void updateTimers();
static void handlePauseAndStats();
static void handleKeys();
static void coreGameUpdate();
static void handleHighlightsAndReplays();
static void gameOver();
static bool gameEnded(TeamGame *topTeam, TeamGame *bottomTeam);
static void updateGameTimersAndCameraBreakMode();
static void pausedLoop();
static void showStatsLoop();
static void loadCrowdChantSampleIfNeeded();
static void initGoalSprites();

static void markPlayer();
static void setCameraMovingToShowerState();
static void firstHalfJustEnded();
static void goToHalftime();
static void prepareForInitialKick();

void gameLoop(TeamGame *topTeam, TeamGame *bottomTeam)
{
    swos.playGame = 1;

    showStadiumScreenAndFadeOutMusic(topTeam, bottomTeam, swos.gameMaxSubstitutes);

    do {
        initGameLoop();
        markFrameStartTime();

        // the really real main game loop ;)
        while (true) {
#ifdef SWOS_TEST
            m_gameLoopStartHook();
#endif
            loadCrowdChantSampleIfNeeded();
            updateTimers();
            handlePauseAndStats();

            handleKeys();

            coreGameUpdate();
            drawFrame(true);

            bool skipUpdate = false;

            if (m_doFadeIn) {
                gameFadeIn();
                m_doFadeIn = false;
                skipUpdate = true;
                swos.currentGameTick = 0;   // for tests (to remain original game compatible)
                swos.lastGameTick = 0;
            }

            handleHighlightsAndReplays();

            if (!swos.playGame) {
                gameFadeOut();
                break;
            }

            if (!skipUpdate)
                updateScreen(true);

#ifdef SWOS_TEST
            m_gameLoopEndHook();
#endif
        }
    } while (!gameEnded(topTeam, bottomTeam));

    m_playingMatch = false;
}

void showStadiumScreenAndFadeOutMusic(TeamGame *topTeam, TeamGame *bottomTeam, int maxSubstitutes)
{
    if (showPreMatchMenus()) {
        showStadiumMenu(topTeam, bottomTeam, maxSubstitutes, [&]() {
            initMatch(topTeam, bottomTeam, true);
        });
    } else {
        initMatch(topTeam, bottomTeam, true);
    }

    waitForMusicToFadeOut();
}

void requestFadeAndSaveReplay()
{
    m_fadeAndSaveReplay = true;
}

void requestFadeAndInstantReplay()
{
    m_fadeAndInstantReplay = true;
}

void requestFadeAndReplayHighlights()
{
    m_fadeAndReplayHighlights = true;
}

bool isMatchRunning()
{
    return m_playingMatch;
}

void setPenaltiesInterval(int interval)
{
    m_penaltiesInterval = interval;
}

void setInitalKickInterval(int interval)
{
    m_initalKickInterval = interval;
}

void setGoalCameraInterval(int interval)
{
    m_goalCameraInterval = interval;
}

void setAllowPlayerControlCameraInterval(int interval)
{
    m_allowPlayerControlCameraInterval = interval;
}

#ifdef SWOS_TEST
void setGameLoopStartHook(std::function<void()> hook)
{
    m_gameLoopStartHook = hook ? hook : []() {};
}

void setGameLoopEndHook(std::function<void()> hook)
{
    m_gameLoopEndHook = hook ? hook : []() {};
}
#endif

static void initGameLoop()
{
    m_playingMatch = true;
    m_doFadeIn = true;

    initGameAudio();
    playCrowdNoise();

    if (swos.playGame)
        playFansChant4lSample();

    flushKeyBuffer();

    initBenchBeforeMatch();

    swos.trainingGameCopy = swos.g_trainingGame;
    swos.gameCanceled = 0;
    swos.saveHighlightScene = 0;
    swos.instantReplayFlag = 0;

    m_fadeAndSaveReplay = false;
    m_fadeAndInstantReplay = false;
    m_fadeAndReplayHighlights = false;

    setCameraToInitialPosition();

    ReadTimerDelta();

    waitForKeyboardAndMouseIdle();

    swos.currentGameTick = 0;
    swos.lastFrameTicks = 0;

    initFrameTicks();

    resetGameControls();
}

static void drawFrame(bool recordingEnabled)
{
    setReplayRecordingEnabled(recordingEnabled);
    float xOffset, yOffset;
    std::tie(xOffset, yOffset) = drawPitchAtCurrentCamera();
    startNewHighlightsFrame();
    drawSprites(xOffset, yOffset);
    drawBench(xOffset, yOffset);
    drawPlayerName();
    drawGameTime();
    drawStatsIfNeeded();
    drawResult();
    drawSpinningLogo();
}

static void gameFadeOut()
{
    fadeOut([]() { drawFrame(false); });
}

static void gameFadeIn()
{
    fadeIn([]() { drawFrame(false); });
}

static void updateTimers()
{
    ReadTimerDelta();

    swos.frameCount++;

    if (swos.spaceReplayTimer)
        swos.spaceReplayTimer--;
}

static void handlePauseAndStats()
{
    pausedLoop();

    if (statsEnqueued())
        showStatsLoop();
}

static void handleKeys()
{
    processControlEvents();
    checkGameKeys();
}

static void coreGameUpdate()
{
    moveCamera();
    playEnqueuedSamples();
    updateGameTime();
    initGoalSprites();
    updateGameTimersAndCameraBreakMode();
    if (!updateFireBlocked()) {
        auto team = selectTeamForUpdate();
        updateTeamControls(team);
        updatePlayers(team);    // main game engine update
        postUpdateTeamControls(team);
    }
    updateBall();
    movePlayers();
    updateReferee();
    updateCornerFlags();
    updateSpinningLogo();
    //ManageAdvertisements();
    DoGoalkeeperSprites();
    updateControlledPlayerNumbers();
    markPlayer();
    updateCurrentPlayerName();
    updateBookedPlayerNumberSprite();
    updateResult();
    SWOS::DrawAnimatedPatterns(); // remove/re-implement
    updateBench();
    updateStatistics();
}

static void handleHighlightsAndReplays()
{
    // TODO: remove when UpdateCameraBreakMode() is converted
    if (swos.saveHighlightScene) {
        saveHighlightScene();
        swos.saveHighlightScene = 0;
    }

    if (m_fadeAndSaveReplay) {
        gameFadeOut();
        saveHighlightScene();
        m_doFadeIn = true;
        m_fadeAndSaveReplay = false;
    }

    // remove instantReplayFlag when UpdateCameraBreakMode() is converted
    if (m_fadeAndInstantReplay || swos.instantReplayFlag) {
        gameFadeOut();

        bool userRequested = true;
        if (swos.instantReplayFlag) {
            userRequested = false;
            swos.instantReplayFlag = 0;
            swos.goalCameraMode = 0;
        }

        playInstantReplay(userRequested);

        m_doFadeIn = true;
        m_fadeAndInstantReplay = false;
        swos.goalCameraMode = 0;    // is this necessary?
    }

    if (m_fadeAndReplayHighlights) {
        gameFadeOut();
        playHighlights(true);
        m_doFadeIn = true;
        m_fadeAndReplayHighlights = false;
    }
}

// Executed when the match finishes.
static void gameOver()
{
    constexpr int kBallOffCourtX = 1672;

    gameFadeOut();

#ifdef SWOS_TEST
    setCameraX(FixedPoint(kGameEndCameraX.whole(), getCameraX().fraction()));
    setCameraY(FixedPoint(kGameEndCameraY.whole(), getCameraY().fraction()));
#else
    setCameraX(kGameEndCameraX);
    setCameraY(kGameEndCameraY);
#endif
    drawPitchAtCurrentCamera();
    setBallPosition(kBallOffCourtX, kPitchCenterY);

    swos.resultTimer = 30'000;
    swos.stoppageEventTimer = 1'650;
    swos.gameState = GameState::kResultAfterTheGame;
    swos.breakCameraMode = -1;
    swos.gameStatePl = GameState::kStopped;
    swos.cameraDirection = -1;
    swos.lastTeamPlayedBeforeBreak = &swos.topTeamData;
    swos.stoppageTimerTotal = 0;
    swos.stoppageTimerActive = 0;

    stopAllPlayers();

    swos.cameraXVelocity = 0;
    swos.cameraYVelocity = 0;

    playEndGameCrowdSampleAndComment();

    m_doFadeIn = true;
}

void SWOS::GameOver()
{
    gameOver();
}

static bool gameEnded(TeamGame *topTeam, TeamGame *bottomTeam)
{
    stopAudio();
    matchEnded();
    refreshReplayGameData();
    setStandardMenuBackgroundImage();

    if (!swos.isGameFriendly || swos.g_trainingGame)
        return true;

    bool replaySelected = showReplayExitMenuAfterFriendly();
    if (!replaySelected)
        return true;

    swos.team1NumAllowedInjuries = 4;
    swos.team2NumAllowedInjuries = 4;

    swos.playGame = 1;

    initMatch(swos.topTeamPtr, swos.bottomTeamPtr, false);

    return false;
}

static void updateGameTimersAndCameraBreakMode()
{
    using namespace SwosVM;

    ax = *(word *)&g_memByte[523148];       // mov ax, playingPenalties
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_no_penalties;                // jz short @@no_penalties

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 31;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTIES
    if (flags.zero)
        goto l_no_penalties;                // jz short @@no_penalties

    {
        word src = *(word *)&g_memByte[523156];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        src = res;
        *(word *)&g_memByte[523156] = src;
    }                                       // add penaltiesTimer, 1
    {
        word src = *(word *)&g_memByte[523156];
        int16_t dstSigned = src;
        int16_t srcSigned = *(word *)&m_penaltiesInterval;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp penaltiesTimer, penaltiesInterval
    if (!flags.zero)
        goto l_no_penalties;                // jnz short @@no_penalties

    nextPenalty();                          // call NextPenalty

l_no_penalties:;
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 100;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_GAME_IN_PROGRESS
    if (!flags.zero)
        goto l_game_not_in_progress;        // jnz short @@game_not_in_progress

    ax = *(word *)&g_memByte[323900];       // mov ax, lastFrameTicks
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word src = *(word *)&g_memByte[523132];
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[523132] = src;
    }                                       // add inGameCounter, ax
    return;                                 // jmp @@out

l_game_not_in_progress:;
    {
        word src = *(word *)&g_memByte[523104];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        src = res;
        *(word *)&g_memByte[523104] = src;
    }                                       // add gameNotInProgressCounterWriteOnly, 1
    ax = *(word *)&g_memByte[323900];       // mov ax, lastFrameTicks
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word src = *(word *)&g_memByte[523112];
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        *(word *)&g_memByte[523112] = src;
    }                                       // add stoppageTimerTotal, ax
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 102;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_WAITING_ON_PLAYER
    if (!flags.zero)
        goto l_not_waiting_on_player;       // jnz short @@not_waiting_on_player

    {
        word src = *(word *)&g_memByte[523114];
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        *(word *)&g_memByte[523114] = src;
    }                                       // add stoppageTimerActive, ax
    eax = *(dword *)&g_memByte[523108];     // mov eax, lastTeamPlayedBeforeBreak
    A6 = eax;                               // mov A6, eax
    esi = A6;                               // mov esi, A6
    ax = (word)readMemory(esi + 4, 2);      // mov ax, [esi+TeamGeneralInfo.playerNumber]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_jmp_out;                     // jnz short @@jmp_out

    {
        word src = *(word *)&g_memByte[523114];
        int16_t dstSigned = src;
        int16_t srcSigned = *(word *)&m_initalKickInterval;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp stoppageTimerActive, initalKickInterval
    if (flags.carry)
        goto l_jmp_out;                     // jb short @@jmp_out

    prepareForInitialKick();                // call PrepareForInitialKick
    {
        word src = *(word *)&g_memByte[455938];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[455938] = src;
    }                                       // add initialKickWriteOnlyTicks, 1
    return;                                 // jmp @@out

l_jmp_out:;
    return;                                 // jmp @@out

l_not_waiting_on_player:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 21;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_STARTING_GAME
    if (flags.carry)
        goto l_game_started;                // jb @@game_started

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 30;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_GAME_ENDED
    if (!flags.carry && !flags.zero)
        goto l_game_started;                // ja @@game_started

    al = g_memByte[522842];                 // mov al, topTeamData.firePressed
    flags.carry = false;
    flags.sign = (al & 0x80) != 0;
    flags.zero = al == 0;                   // or al, al
    if (!flags.zero)
        goto l_end_game_transmission;       // jnz @@end_game_transmission

    al = g_memByte[522990];                 // mov al, bottomTeamData.firePressed
    flags.carry = false;
    flags.sign = (al & 0x80) != 0;
    flags.zero = al == 0;                   // or al, al
    if (!flags.zero)
        goto l_end_game_transmission;       // jnz short @@end_game_transmission

    {
        word src = *(word *)&g_memByte[522798];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp topTeamData.playerCoachNumber, 1
    if (flags.zero)
        goto l_check_pl1_fire;              // jz short @@check_pl1_fire

    {
        word src = *(word *)&g_memByte[522946];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp bottomTeamData.playerCoachNumber, 1
    if (!flags.zero)
        goto l_check_for_pl2_coach;         // jnz short @@check_for_pl2_coach

l_check_pl1_fire:;
    SWOS::Player1StatusProc();              // call Player1StatusProc
    al = g_memByte[323920];                 // mov al, pl1Fire
    flags.carry = false;
    flags.sign = (al & 0x80) != 0;
    flags.zero = al == 0;                   // or al, al
    if (!flags.zero)
        goto l_end_game_transmission;       // jnz short @@end_game_transmission

l_check_for_pl2_coach:;
    {
        word src = *(word *)&g_memByte[522798];
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp topTeamData.playerCoachNumber, 2
    if (flags.zero)
        goto l_check_pl2_fire;              // jz short @@check_pl2_fire

    {
        word src = *(word *)&g_memByte[522946];
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp bottomTeamData.playerCoachNumber, 2
    if (!flags.zero)
        goto l_check_control_states_again;  // jnz short @@check_control_states_again

l_check_pl2_fire:;
    SWOS::Player2StatusProc();              // call Player2StatusProc
    al = g_memByte[323921];                 // mov al, pl2Fire
    flags.carry = false;
    flags.sign = (al & 0x80) != 0;
    flags.zero = al == 0;                   // or al, al
    if (!flags.zero)
        goto l_end_game_transmission;       // jnz short @@end_game_transmission

l_check_control_states_again:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 26;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_RESULT_AFTER_THE_GAME
    if (!flags.zero)
        goto l_game_started;                // jnz @@game_started

    SWOS::Player1StatusProc();              // call Player1StatusProc
    al = g_memByte[323920];                 // mov al, pl1Fire
    flags.carry = false;
    flags.sign = (al & 0x80) != 0;
    flags.zero = al == 0;                   // or al, al
    if (!flags.zero)
        goto l_end_game_transmission;       // jnz short @@end_game_transmission

    SWOS::Player2StatusProc();              // call Player2StatusProc
    al = g_memByte[323921];                 // mov al, pl2Fire
    flags.carry = false;
    flags.sign = (al & 0x80) != 0;
    flags.zero = al == 0;                   // or al, al
    if (flags.zero)
        goto l_game_started;                // jz @@game_started

l_end_game_transmission:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 25;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_RESULT_ON_HALFTIME
    if (!flags.zero)
        goto l_not_half_time_result;        // jnz short @@not_half_time_result

    {
        int16_t src = *(word *)&g_memByte[449742];
        src = -src;
        *(word *)&g_memByte[449742] = src;
    }                                       // neg resultTimer
    {
        int16_t src = *(word *)&g_memByte[449744];
        src = -src;
        *(word *)&g_memByte[449744] = src;
    }                                       // neg timeVar
    {
        int16_t src = *(word *)&g_memByte[449740];
        flags.carry = src != 0;
        src = -src;
        flags.sign = (src & 0x8000) != 0;
        flags.zero = src == 0;
        *(word *)&g_memByte[449740] = src;
    }                                       // neg statsTimer
    setCameraMovingToShowerState();         // call SetCameraMovingToShowerState
    prepareForInitialKick();                // call PrepareForInitialKick
    *(word *)&g_memByte[523122] = 0;        // mov stoppageEventTimer, 0
    return;                                 // jmp @@out

l_not_half_time_result:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 26;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_RESULT_AFTER_THE_GAME
    if (!flags.zero)
        goto l_not_result_after_the_game;   // jnz short @@not_result_after_the_game

    {
        int16_t src = *(word *)&g_memByte[449742];
        src = -src;
        *(word *)&g_memByte[449742] = src;
    }                                       // neg resultTimer
    {
        int16_t src = *(word *)&g_memByte[449744];
        src = -src;
        *(word *)&g_memByte[449744] = src;
    }                                       // neg timeVar
    {
        int16_t src = *(word *)&g_memByte[449740];
        flags.carry = src != 0;
        src = -src;
        flags.sign = (src & 0x8000) != 0;
        flags.zero = src == 0;
        *(word *)&g_memByte[449740] = src;
    }                                       // neg statsTimer
    *(word *)&g_memByte[252376] = 0;        // mov playGame, 0
    *(word *)&g_memByte[523122] = 0;        // mov stoppageEventTimer, 0
    return;                                 // jmp @@out

l_not_result_after_the_game:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 21;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_STARTING_GAME
    if (!flags.zero)
        goto l_game_not_starting;           // jnz short @@game_not_starting

    *(word *)&g_memByte[521018] = 0;        // mov showFansCounter, 0
    prepareForInitialKick();                // call PrepareForInitialKick
    *(word *)&g_memByte[523122] = 0;        // mov stoppageEventTimer, 0
    return;                                 // jmp @@out

l_game_not_starting:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 22;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_CAMERA_GOING_TO_SHOWERS
    if (!flags.zero)
        goto l_game_started;                // jnz short @@game_started

    prepareForInitialKick();                // call PrepareForInitialKick
    *(word *)&g_memByte[523122] = 0;        // mov stoppageEventTimer, 0
    return;                                 // jmp @@out

l_game_started:;
    ax = *(word *)&g_memByte[523122];       // mov ax, stoppageEventTimer
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_stoppage_event_triggered;    // jz short @@stoppage_event_triggered

    ax = *(word *)&g_memByte[323900];       // mov ax, lastFrameTicks
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    {
        word src = *(word *)&g_memByte[523122];
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[523122] = src;
    }                                       // sub stoppageEventTimer, ax
    if (flags.sign)
        goto l_stoppage_event_triggered;    // js short @@stoppage_event_triggered

    if (!flags.zero)
        return;                             // jnz @@out

l_stoppage_event_triggered:;
    *(word *)&g_memByte[523122] = 0;        // mov stoppageEventTimer, 0
    eax = *(dword *)&g_memByte[523108];     // mov eax, lastTeamPlayedBeforeBreak
    A6 = eax;                               // mov A6, eax
    A5 = 328988;                            // mov A5, offset ballSprite
    {
        word src = *(word *)&g_memByte[523116];
        int16_t dstSigned = src;
        int16_t srcSigned = 101;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameStatePl, ST_STOPPED
    if (!flags.zero)
        goto l_game_running;                // jnz @@game_running

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 25;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_RESULT_ON_HALFTIME
    if (!flags.zero)
        goto l_not_result_on_halftime;      // jnz short @@not_result_on_halftime

    {
        int16_t src = *(word *)&g_memByte[449742];
        src = -src;
        *(word *)&g_memByte[449742] = src;
    }                                       // neg resultTimer
    {
        int16_t src = *(word *)&g_memByte[449744];
        src = -src;
        *(word *)&g_memByte[449744] = src;
    }                                       // neg timeVar
    {
        int16_t src = *(word *)&g_memByte[449740];
        flags.carry = src != 0;
        src = -src;
        flags.sign = (src & 0x8000) != 0;
        flags.zero = src == 0;
        *(word *)&g_memByte[449740] = src;
    }                                       // neg statsTimer
    setCameraMovingToShowerState();         // call SetCameraMovingToShowerState
    return;                                 // jmp @@out

l_not_result_on_halftime:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 26;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_RESULT_AFTER_THE_GAME
    if (!flags.zero)
        goto l_not_showing_end_game_result; // jnz short @@not_showing_end_game_result

    {
        int16_t src = *(word *)&g_memByte[449742];
        src = -src;
        *(word *)&g_memByte[449742] = src;
    }                                       // neg resultTimer
    {
        int16_t src = *(word *)&g_memByte[449744];
        src = -src;
        *(word *)&g_memByte[449744] = src;
    }                                       // neg timeVar
    {
        int16_t src = *(word *)&g_memByte[449740];
        flags.carry = src != 0;
        src = -src;
        flags.sign = (src & 0x8000) != 0;
        flags.zero = src == 0;
        *(word *)&g_memByte[449740] = src;
    }                                       // neg statsTimer
    *(word *)&g_memByte[252376] = 0;        // mov playGame, 0
    return;                                 // jmp @@out

l_not_showing_end_game_result:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 21;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_STARTING_GAME
    if (!flags.zero)
        goto l_not_starting_the_game;       // jnz short @@not_starting_the_game

    prepareForInitialKick();                // call PrepareForInitialKick
    return;                                 // jmp @@out

l_not_starting_the_game:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 22;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_CAMERA_GOING_TO_SHOWERS
    if (!flags.zero)
        goto l_not_going_to_showers;        // jnz short @@not_going_to_showers

    prepareForInitialKick();                // call PrepareForInitialKick
    return;                                 // jmp @@out

l_not_going_to_showers:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 29;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_FIRST_HALF_ENDED
    if (!flags.zero)
        goto l_not_1st_half_ended;          // jnz short @@not_1st_half_ended

    firstHalfJustEnded();                   // call FirstHalfJustEnded
    return;                                 // jmp @@out

l_not_1st_half_ended:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 23;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_GOING_TO_HALFTIME
    if (!flags.zero)
        goto l_not_going_to_halftime;       // jnz short @@not_going_to_halftime

    {
        int16_t src = *(word *)&g_memByte[449742];
        src = -src;
        *(word *)&g_memByte[449742] = src;
    }                                       // neg resultTimer
    {
        int16_t src = *(word *)&g_memByte[449744];
        src = -src;
        *(word *)&g_memByte[449744] = src;
    }                                       // neg timeVar
    {
        int16_t src = *(word *)&g_memByte[449740];
        flags.carry = src != 0;
        src = -src;
        flags.sign = (src & 0x8000) != 0;
        flags.zero = src == 0;
        *(word *)&g_memByte[449740] = src;
    }                                       // neg statsTimer
    goToHalftime();                         // call GoToHalftime
    return;                                 // jmp @@out

l_not_going_to_halftime:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 30;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_GAME_ENDED
    if (!flags.zero)
        goto l_game_not_ended;              // jnz short @@game_not_ended

    playersLeavingPitch();                  // call PlayersLeavingPitch
    return;                                 // jmp @@out

l_game_not_ended:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 24;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PLAYERS_GOING_TO_SHOWER
    if (!flags.zero)
        goto l_players_not_going_to_showers; // jnz short @@players_not_going_to_showers

    SWOS::GameOver();                       // call GameOver
    return;                                 // jmp @@out

l_players_not_going_to_showers:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 27;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_FIRST_EXTRA_STARTING
    if (!flags.zero)
        goto l_first_extra_not_starting;    // jnz short @@first_extra_not_starting

    prepareForInitialKick();                // call PrepareForInitialKick
    return;                                 // jmp @@out

l_first_extra_not_starting:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 28;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_FIRST_EXTRA_ENDED
    if (!flags.zero)
        goto l_first_extra_not_ended;       // jnz short @@first_extra_not_ended

    prepareForInitialKick();                // call PrepareForInitialKick
    return;                                 // jmp @@out

l_first_extra_not_ended:;
    ax = *(word *)&g_memByte[523118];       // mov ax, gameState
    *(word *)&g_memByte[523116] = ax;       // mov gameStatePl, ax
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (!flags.zero)
        goto cseg_73993;                    // jnz short cseg_73993

    ax = *(word *)&g_memByte[486142];       // mov ax, g_inSubstitutesMenu
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto cseg_73993;                    // jnz short cseg_73993

    ax = *(word *)&g_memByte[486144];       // mov ax, g_cameraLeavingSubsTimer
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto cseg_73993;                    // jnz short cseg_73993

    *(word *)&g_memByte[449744] = 31000;    // mov timeVar, 31000

cseg_73993:;
    *(word *)&g_memByte[523120] = 0;        // mov breakCameraMode, 0
    return;                                 // jmp @@out

l_game_running:;
    ax = *(word *)&g_memByte[523120];       // mov ax, breakCameraMode
    *(word *)&D7 = ax;                      // mov word ptr D7, ax
    ax = *(word *)&g_memByte[486142];       // mov ax, g_inSubstitutesMenu
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        return;                             // jnz @@out

    ax = *(word *)&g_memByte[486146];       // mov ax, g_waitForPlayerToGoInTimer
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        return;                             // jnz @@out

    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 0;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 0
    if (!flags.zero)
        goto cseg_73A30;                    // jnz short cseg_73A30

    esi = A5;                               // mov esi, A5
    eax = readMemory(esi + 46, 4);          // mov eax, [esi+Sprite.deltaX]
    flags.carry = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.zero)
        return;                             // jnz @@out

    eax = readMemory(esi + 50, 4);          // mov eax, [esi+Sprite.deltaY]
    flags.carry = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (!flags.zero)
        return;                             // jnz @@out

    *(word *)&g_memByte[523120] = 1;        // mov breakCameraMode, 1
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        return;                             // jz @@out

    *(word *)&g_memByte[523122] = *(word *)&m_goalCameraInterval; // mov stoppageEventTimer, goalCameraInterval
    ax = *(word *)&g_memByte[455982];       // mov ax, goalCameraMode
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        return;                             // jz @@out

    *(word *)&g_memByte[523122] = 75;       // mov stoppageEventTimer, 75
    return;                                 // retn

cseg_73A30:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 1
    if (!flags.zero)
        goto cseg_73B28;                    // jnz cseg_73B28

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 14;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTY
    if (flags.zero)
        goto l_game_stopped;                // jz short @@game_stopped

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 31;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTIES
    if (flags.zero)
        goto l_game_stopped;                // jz short @@game_stopped

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        goto l_game_stopped;                // jz short @@game_stopped

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 13;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_FOUL
    if (flags.zero)
        goto l_game_stopped;                // jz short @@game_stopped

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 6;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_FREE_KICK_LEFT1
    if (flags.carry)
        goto l_game_stopped;                // jb short @@game_stopped

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 12;
        word res = dstSigned - srcSigned;
    }                                       // cmp gameState, ST_FREE_KICK_RIGHT3

l_game_stopped:;
    ax = *(word *)&g_memByte[523148];       // mov ax, playingPenalties
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_no_auto_replay;              // jnz short @@no_auto_replay

    ax = *(word *)&g_memByte[486142];       // mov ax, g_inSubstitutesMenu
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto l_no_auto_replay;              // jnz short @@no_auto_replay

    ax = *(word *)&g_memByte[455982];       // mov ax, goalCameraMode
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_no_auto_replay;              // jz short @@no_auto_replay

    ax = *(word *)&g_memByte[131704];       // mov ax, g_autoSaveHighlights
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_no_auto_save_highlights;     // jz short @@no_auto_save_highlights

    *(word *)&g_memByte[486134] = 1;        // mov saveHighlightScene, 1

l_no_auto_save_highlights:;
    enqueueCrowdChantsReload();       // call EnqueueCrowdChantsReload
    ax = *(word *)&g_memByte[131700];       // mov ax, g_autoReplays
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_no_auto_replay;              // jz short @@no_auto_replay

    *(word *)&g_memByte[455924] = 0;        // mov userRequestedReplay, 0
    *(word *)&g_memByte[486132] = 1;        // mov instantReplayFlag, 1
    *(word *)&g_memByte[456256] = 1;        // mov loadCrowdChantSampleFlag, 1

l_no_auto_replay:;
    *(dword *)&g_memByte[522748] = 0;       // mov currentScorer, 0
    ax = *(word *)&g_memByte[523148];       // mov ax, playingPenalties
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto cseg_73AF7;                    // jz short cseg_73AF7

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 31;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTIES
    if (flags.zero)
        goto cseg_73AF7;                    // jz short cseg_73AF7

    return;                                 // jmp @@out

cseg_73AF7:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        goto l_keeper_holds_the_ball;       // jz short @@keeper_holds_the_ball

    ax = *(word *)&g_memByte[523124];       // mov ax, foulXCoordinate
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = *(word *)&g_memByte[523126];       // mov ax, foulYCoordinate
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    setBallPosition(D1.asWord(), D2.asWord()); // call SetBallPosition

l_keeper_holds_the_ball:;
    *(word *)&g_memByte[523120] = 2;        // mov breakCameraMode, 2
    return;                                 // retn

cseg_73B28:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 2
    if (!flags.zero)
        goto cseg_73C3D;                    // jnz cseg_73C3D

    ax = *(word *)&g_memByte[523638];       // mov ax, whichCard
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        return;                             // jnz @@out

    ax = *(word *)&g_memByte[455926];       // mov ax, cameraCoordinatesValid
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        return;                             // jz @@out

    *(word *)&g_memByte[457524] = 0;        // mov goalScored, 0
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (!flags.zero)
        goto cseg_73B85;                    // jnz short cseg_73B85

    *(word *)&g_memByte[523116] = 102;      // mov gameStatePl, ST_WAITING_ON_PLAYER
    *(word *)&g_memByte[523132] = 0;        // mov inGameCounter, 0
    ax = *(word *)&g_memByte[523118];       // mov ax, gameState
    *(word *)&g_memByte[523134] = ax;       // mov breakState, ax

cseg_73B85:;
    ax = *(word *)&g_memByte[522934];       // mov ax, topTeamData.resetControls
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto cseg_73BCE;                    // jnz short cseg_73BCE

    eax = *(dword *)&g_memByte[522812];     // mov eax, topTeamData.spritesTable
    A0 = eax;                               // mov A0, eax
    *(word *)&D0 = 10;                      // mov word ptr D0, 10

l_next_player_sprite:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi, 4);               // mov eax, [esi]
    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 4;
        dword res = dstSigned + srcSigned;
        A0 = res;
    }                                       // add A0, 4
    A1 = eax;                               // mov A1, eax
    flags.carry = false;                    // or eax, eax
    esi = A1;                               // mov esi, A1
    writeMemory(esi + 100, 2, 1);           // mov [esi+Sprite.destReachedState], 1
    (*(int16_t *)&D0)--;
    flags.sign = (*(int16_t *)&D0 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D0 == 0;      // dec word ptr D0
    if (!flags.sign)
        goto l_next_player_sprite;          // jns short @@next_player_sprite

cseg_73BCE:;
    ax = *(word *)&g_memByte[523082];       // mov ax, bottomTeamData.resetControls
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        goto cseg_73C17;                    // jnz short cseg_73C17

    eax = *(dword *)&g_memByte[522960];     // mov eax, bottomTeamData.spritesTable
    A0 = eax;                               // mov A0, eax
    *(word *)&D0 = 10;                      // mov word ptr D0, 10

l_players_loop:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi, 4);               // mov eax, [esi]
    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 4;
        dword res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        A0 = res;
    }                                       // add A0, 4
    A1 = eax;                               // mov A1, eax
    esi = A1;                               // mov esi, A1
    writeMemory(esi + 100, 2, 1);           // mov [esi+Sprite.destReachedState], 1
    (*(int16_t *)&D0)--;
    flags.sign = (*(int16_t *)&D0 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D0 == 0;      // dec word ptr D0
    if (!flags.sign)
        goto l_players_loop;                // jns short @@players_loop

cseg_73C17:;
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (!flags.zero)
        goto cseg_73C33;                    // jnz short cseg_73C33

    *(word *)&g_memByte[326624] = 3;        // mov goalie1Sprite.destReachedState, 3
    *(word *)&g_memByte[327856] = 3;        // mov goalie2Sprite.destReachedState, 3

cseg_73C33:;
    *(word *)&g_memByte[523120] = 4;        // mov breakCameraMode, 4
    return;                                 // retn

cseg_73C3D:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 4;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 4
    if (!flags.zero)
        goto cseg_73C60;                    // jnz short cseg_73C60

    ax = *(word *)&g_memByte[486142];       // mov ax, g_inSubstitutesMenu
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        return;                             // jnz @@out

    *(word *)&g_memByte[523120] = 3;        // mov breakCameraMode, 3
    return;                                 // retn

cseg_73C60:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 3
    if (!flags.zero)
        goto cseg_73DD5;                    // jnz cseg_73DD5

    ax = *(word *)&g_memByte[523620];       // mov ax, refState
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        return;                             // jnz @@out

    ax = *(word *)&g_memByte[523622];       // mov ax, injuriesForever
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        return;                             // jnz @@out

    A0 = 330096;                            // mov A0, offset team1SpritesTable
    *(word *)&D0 = 21;                      // mov word ptr D0, 21

l_check_if_players_arrived_loop:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi, 4);               // mov eax, [esi]
    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 4;
        dword res = dstSigned + srcSigned;
        A0 = res;
    }                                       // add A0, 4
    A1 = eax;                               // mov A1, eax
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 0;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PLAYERS_TO_INITIAL_POSITIONS
    if (flags.zero)
        goto l_check_if_player_arrived;     // jz short @@check_if_player_arrived

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 14;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTY
    if (flags.zero)
        goto l_check_if_player_arrived;     // jz short @@check_if_player_arrived

    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 31;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_PENALTIES
    if (flags.zero)
        goto l_check_if_player_arrived;     // jz short @@check_if_player_arrived

    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + 84, 2);     // mov ax, [esi+Sprite.onScreen]
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto l_check_if_next_player_arrived; // jz short @@check_if_next_player_arrived

l_check_if_player_arrived:;
    esi = A1;                               // mov esi, A1
    {
        word src = (word)readMemory(esi + 100, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.destReachedState], 3
    if (!flags.zero)
        return;                             // jnz @@out

l_check_if_next_player_arrived:;
    (*(int16_t *)&D0)--;
    flags.sign = (*(int16_t *)&D0 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D0 == 0;      // dec word ptr D0
    if (!flags.sign)
        goto l_check_if_players_arrived_loop; // jns short @@check_if_players_arrived_loop

    *(word *)&g_memByte[457526] = 0;        // mov runSlower, 0
    ax = *(word *)&g_memByte[455982];       // mov ax, goalCameraMode
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto cseg_73DCB;                    // jz cseg_73DCB

    push(D0);                               // push D0
    push(D1);                               // push D1
    push(D2);                               // push D2
    push(D3);                               // push D3
    push(D4);                               // push D4
    push(D5);                               // push D5
    push(D6);                               // push D6
    push(D7);                               // push D7
    push(A0);                               // push A0
    push(A1);                               // push A1
    push(A2);                               // push A2
    push(A3);                               // push A3
    push(A4);                               // push A4
    push(A5);                               // push A5
    push(A6);                               // push A6
    //ResetAnimatedPatternsForBothTeams();    // call ResetAnimatedPatternsForBothTeams
    pop(A6);                                // pop A6
    pop(A5);                                // pop A5
    pop(A4);                                // pop A4
    pop(A3);                                // pop A3
    pop(A2);                                // pop A2
    pop(A1);                                // pop A1
    pop(A0);                                // pop A0
    pop(D7);                                // pop D7
    pop(D6);                                // pop D6
    pop(D5);                                // pop D5
    pop(D4);                                // pop D4
    pop(D3);                                // pop D3
    pop(D2);                                // pop D2
    pop(D1);                                // pop D1
    pop(D0);                                // pop D0

cseg_73DCB:;
    *(word *)&g_memByte[523120] = 5;        // mov breakCameraMode, 5
    return;                                 // retn

cseg_73DD5:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 5;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 5
    if (!flags.zero)
        goto cseg_73DFC;                    // jnz short cseg_73DFC

    *(word *)&g_memByte[523638] = 0;        // mov whichCard, 0
    *(dword *)&g_memByte[523640] = 0;       // mov bookedPlayer, 0
    *(word *)&g_memByte[523120] = 6;        // mov breakCameraMode, 6
    return;                                 // retn

cseg_73DFC:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 6;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 6
    if (!flags.zero)
        goto cseg_73E2C;                    // jnz short cseg_73E2C

    *(word *)&g_memByte[523652] = 0;        // mov ballOutOfGameTimer, 0
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        goto cseg_73E22;                    // jz short cseg_73E22

    *(word *)&g_memByte[449744] = 31000;    // mov timeVar, 31000

cseg_73E22:;
    *(word *)&g_memByte[523120] = 7;        // mov breakCameraMode, 7
    return;                                 // retn

cseg_73E2C:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 7;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 7
    if (!flags.zero)
        goto cseg_73F0A;                    // jnz cseg_73F0A

    esi = A6;                               // mov esi, A6
    writeMemory(esi + 94, 2, 1);            // mov [esi+TeamGeneralInfo.ballInPlay], 1
    {
        word src = *(word *)&g_memByte[523652];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        src = res;
        *(word *)&g_memByte[523652] = src;
    }                                       // add ballOutOfGameTimer, 1
    {
        word src = *(word *)&g_memByte[523652];
        int16_t dstSigned = src;
        int16_t srcSigned = *(word *)&m_allowPlayerControlCameraInterval;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp ballOutOfGameTimer, letPlayerControlCameraInterval
    if (flags.carry)
        goto cseg_73E8B;                    // jb short cseg_73E8B

    {
        word src = *(word *)&g_memByte[455952];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[455952] = src;
    }                                       // add deadCameraBreakVar01, 1
    ax = *(word *)&g_memByte[523116];       // mov ax, gameStatePl
    *(word *)&g_memByte[455954] = ax;       // mov deadCameraBreakVar02, ax
    ax = *(word *)&g_memByte[523118];       // mov ax, gameState
    *(word *)&g_memByte[455956] = ax;       // mov deadCameraBreakVar03, ax
    checkIfGoalkeeperClaimedTheBall();      // call CheckIfGoalkeeperClaimedTheBall
    {
        word src = *(word *)&g_memByte[455932];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[455932] = src;
    }                                       // add deadCameraBreakVar04, 1
    return;                                 // jmp @@out

cseg_73E8B:;
    esi = A6;                               // mov esi, A6
    eax = readMemory(esi + 32, 4);          // mov eax, [esi+TeamGeneralInfo.controlledPlayer]
    flags.carry = false;
    flags.sign = (eax & 0x80000000) != 0;
    flags.zero = eax == 0;                  // or eax, eax
    if (flags.zero)
        return;                             // jz @@out

    eax = readMemory(esi + 32, 4);          // mov eax, [esi+TeamGeneralInfo.controlledPlayer]
    A1 = eax;                               // mov A1, eax
    esi = A1;                               // mov esi, A1
    {
        byte src = (byte)readMemory(esi + 12, 1);
        int8_t dstSigned = src;
        int8_t srcSigned = 5;
        byte res = dstSigned - srcSigned;
        flags.carry = static_cast<uint8_t>(dstSigned) < static_cast<uint8_t>(srcSigned);
        flags.sign = (res & 0x80) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.playerState], PL_THROW_IN
    if (flags.zero)
        goto l_set_throw_in_anim_table;     // jz short @@set_throw_in_anim_table

    A0 = 453234;                            // mov A0, offset playerNormalStandingAnimTable
    SetPlayerAnimationTable();              // call SetPlayerAnimationTable
    goto cseg_73ED6;                        // jmp short cseg_73ED6

l_set_throw_in_anim_table:;
    A0 = 453788;                            // mov A0, offset aboutToThrowInAnimTable
    SetPlayerAnimationTable();              // call SetPlayerAnimationTable

cseg_73ED6:;
    esi = A6;                               // mov esi, A6
    writeMemory(esi + 96, 2, 1);            // mov [esi+TeamGeneralInfo.ballOutOfPlay], 1
    *(word *)&g_memByte[523120] = 8;        // mov breakCameraMode, 8
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        return;                             // jz short @@out

    ax = *(word *)&g_memByte[486142];       // mov ax, g_inSubstitutesMenu
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (!flags.zero)
        return;                             // jnz short @@out

    *(word *)&g_memByte[449742] = 31000;    // mov resultTimer, 31000
    return;                                 // retn

cseg_73F0A:;
    {
        int16_t dstSigned = *(word *)&D7;
        int16_t srcSigned = 8;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D7, 8
    if (!flags.zero)
        goto l_assert_failed;               // jnz short @@assert_failed

    *(word *)&g_memByte[523654] = 0;        // mov writeOnlyVar03, 0
    {
        word src = *(word *)&g_memByte[523118];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_KEEPER_HOLDS_BALL
    if (flags.zero)
        goto cseg_73F2C;                    // jz short cseg_73F2C

    SWOS::PlayRefereeWhistleSample();       // call PlayRefereeWhistleSample

cseg_73F2C:;
    *(word *)&g_memByte[455982] = 0;        // mov goalCameraMode, 0
    *(word *)&g_memByte[523116] = 102;      // mov gameStatePl, 102
    *(word *)&g_memByte[523132] = 0;        // mov inGameCounter, 0
    ax = *(word *)&g_memByte[523118];       // mov ax, gameState
    *(word *)&g_memByte[523134] = ax;       // mov breakState, ax

l_out:;
    return;                                 // retn

l_assert_failed:;
    debugBreak();                           // int 3

l_endless_loop:;
    goto l_endless_loop;                    // jmp short @@endless_loop
}

static void pausedLoop()
{
    int oldWidth = -1, oldHeight = -1;

    while (isGamePaused()) {
        processControlEvents();

        int width, height;
        std::tie(width, height) = getWindowSize();

        if (checkGameKeys() || width != oldWidth || height != oldHeight) {
            oldWidth = width;
            oldHeight = height;
            markFrameStartTime();
            drawFrame(false);
            updateScreen(true);
            continue;
        }

        auto events = getPlayerEvents(kPlayer1) | getPlayerEvents(kPlayer2);
        if (events & (kGameEventKick | kGameEventPause)) {
            togglePause();
            break;
        }

        SDL_Delay(100);

        // make sure to reset the timer or delta time will be heavily skewed after the pause
        markFrameStartTime();
    }
}

static void showStatsLoop()
{
    drawFrame(false);
    updateScreen();

    do {
        SDL_Delay(100);
        processControlEvents();
        checkGameKeys();
        if (isAnyPlayerFiring()) {
            swos.fireBlocked = 1;
            hideStats();
        }
    } while (showingUserRequestedStats());
}

static void loadCrowdChantSampleIfNeeded()
{
    if (swos.loadCrowdChantSampleFlag) {
        loadCrowdChantSample();
        swos.loadCrowdChantSampleFlag = 0;
    }
}

static void initGoalSprites()
{
    swos.goal1TopSprite.setImage(kTopGoalSprite);
    swos.goal2BottomSprite.setImage(kBottomGoalSprite);
}

using namespace SwosVM;

static void markPlayer()
{
    A1 = 522792;                            // mov A1, offset topTeamData
    {
        byte src = g_memByte[323904];
        byte res = src & 16;
        flags.carry = false;
        flags.sign = (res & 0x80) != 0;
        flags.zero = res == 0;
    }                                       // test byte ptr currentGameTick, 10h
    if (flags.zero)
        goto l_do_mark;                     // jz short @@do_mark

    A1 = 522940;                            // mov A1, offset bottomTeamData

l_do_mark:;
    esi = A1;                               // mov esi, A1
    eax = readMemory(esi + 10, 4);          // mov eax, [esi+TeamGeneralInfo.inGameTeamPtr]
    A2 = eax;                               // mov A2, eax
    A0 = 325968;                            // mov A0, offset playerMarkSprite
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 70, 2, -1);           // mov [esi+Sprite.imageIndex], -1
    esi = A2;                               // mov esi, A2
    ax = (word)readMemory(esi + 20, 2);     // mov ax, [esi+TeamGame.markedPlayer]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.sign)
        return;                             // js @@out

    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 11;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 11
    if (!flags.carry)
        return;                             // jnb @@out

    esi = A1;                               // mov esi, A1
    eax = readMemory(esi + 20, 4);          // mov eax, [esi+TeamGeneralInfo.spritesTable]
    A1 = eax;                               // mov A1, eax
    {
        word res = *(word *)&D0 << 2;
        *(word *)&D0 = res;
    }                                       // shl word ptr D0, 2
    esi = A1;                               // mov esi, A1
    ebx = *(word *)&D0;                     // movzx ebx, word ptr D0
    eax = readMemory(esi + ebx, 4);         // mov eax, [esi+ebx]
    A1 = eax;                               // mov A1, eax
    esi = A1;                               // mov esi, A1
    {
        byte src = (byte)readMemory(esi + 12, 1);
        int8_t dstSigned = src;
        int8_t srcSigned = 0;
        byte res = dstSigned - srcSigned;
        flags.carry = static_cast<uint8_t>(dstSigned) < static_cast<uint8_t>(srcSigned);
        flags.sign = (res & 0x80) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+Sprite.playerState], PL_NORMAL
    if (!flags.zero)
        return;                             // jnz short @@out

    esi = A0;                               // mov esi, A0
    writeMemory(esi + 70, 2, 1204);         // mov [esi+Sprite.imageIndex], SPR_PLAYER_MARK
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    ax = (word)readMemory(esi + 40, 2);     // mov ax, word ptr [esi+(Sprite.z+2)]
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    {
        int16_t dstSigned = *(word *)&D2;
        int16_t srcSigned = 20;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D2 = res;
    }                                       // add word ptr D2, 20
    ax = D1;                                // mov ax, word ptr D1
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 32, 2, ax);           // mov word ptr [esi+(Sprite.x+2)], ax
    ax = D2;                                // mov ax, word ptr D2
    writeMemory(esi + 40, 2, ax);           // mov word ptr [esi+(Sprite.z+2)], ax
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 36, 2, ax);           // mov word ptr [esi+(Sprite.y+2)], ax
}

static void setCameraMovingToShowerState()
{
    *(word *)&g_memByte[523142] = 2;        // mov halfNumber, 2
    {
        int16_t src = *(word *)&g_memByte[523146];
        src = -src;
        *(word *)&g_memByte[523146] = src;
    }                                       // neg teamPlayingUp
    {
        word src = *(word *)&g_memByte[523146];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned + srcSigned;
        src = res;
        *(word *)&g_memByte[523146] = src;
    }                                       // add teamPlayingUp, 3
    {
        int16_t src = *(word *)&g_memByte[523144];
        src = -src;
        *(word *)&g_memByte[523144] = src;
    }                                       // neg teamStarting
    {
        word src = *(word *)&g_memByte[523144];
        int16_t dstSigned = src;
        int16_t srcSigned = 3;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[523144] = src;
    }                                       // add teamStarting, 3
    *(word *)&D1 = 1672;                    // mov word ptr D1, 1672
    *(word *)&D2 = 449;                     // mov word ptr D2, 449
    setBallPosition(D1.asWord(), D2.asWord()); // call SetBallPosition
    *(word *)&g_memByte[449800] = 0;        // mov hideBall, 0
    initTeamsData();                        // call InitTeamsData
    *(word *)&g_memByte[523122] = 110;      // mov stoppageEventTimer, 110
    *(word *)&g_memByte[523118] = 22;       // mov gameState, ST_CAMERA_GOING_TO_SHOWERS
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, 101
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    *(word *)&g_memByte[523128] = -1;       // mov cameraDirection, -1
    *(dword *)&g_memByte[523108] = 522792;  // mov lastTeamPlayedBeforeBreak, offset topTeamData
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0
    ax = *(word *)&g_memByte[252376];       // mov ax, playGame
    flags.carry = false;
    flags.sign = (ax & 0x8000) != 0;
    flags.zero = ax == 0;                   // or ax, ax
    if (flags.zero)
        goto cseg_75B53;                    // jz short cseg_75B53

    *(word *)&g_memByte[449746] = 385;      // mov nullSampleTimer, 385

cseg_75B53:;
    initPlayersBeforeEnteringPitch();       // jmp InitPlayersBeforeEnteringPitch
}

static void firstHalfJustEnded()
{
    *(word *)&g_memByte[449800] = 0;        // mov hideBall, 0
    *(word *)&g_memByte[523122] = 275;      // mov stoppageEventTimer, 275
    *(word *)&g_memByte[523118] = 23;       // mov gameState, ST_GOING_TO_HALFTIME
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, ST_STOPPED
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    *(word *)&g_memByte[523128] = -1;       // mov cameraDirection, -1
    *(dword *)&g_memByte[523108] = 522792;  // mov lastTeamPlayedBeforeBreak, offset topTeamData
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0
    *(word *)&g_memByte[456262] = 0;        // mov stateGoal, 0
}

static void goToHalftime()
{
    *(word *)&D1 = 1672;                    // mov word ptr D1, 1672
    *(word *)&D2 = 449;                     // mov word ptr D2, 449
    setBallPosition(D1.asWord(), D2.asWord()); // call SetBallPosition
    *(word *)&g_memByte[449742] = 30000;    // mov resultTimer, 30000
    *(word *)&g_memByte[449744] = 32000;    // mov timeVar, 32000
    *(word *)&g_memByte[523122] = 770;      // mov stoppageEventTimer, 770
    *(word *)&g_memByte[523118] = 25;       // mov gameState, ST_RESULT_ON_HALFTIME
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, 101
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    *(word *)&g_memByte[523128] = -1;       // mov cameraDirection, -1
    *(dword *)&g_memByte[523108] = 522792;  // mov lastTeamPlayedBeforeBreak, offset topTeamData
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0
}

static void prepareForInitialKick()
{
    *(word *)&D1 = 336;                     // mov word ptr D1, 336
    *(word *)&D2 = 449;                     // mov word ptr D2, 449
    setBallPosition(D1.asWord(), D2.asWord()); // call SetBallPosition
    A0 = 522792;                            // mov A0, offset topTeamData
    *(word *)&D1 = 4;                       // mov word ptr D1, 4
    *(byte *)&D2 = 124;                     // mov byte ptr D2, 7Ch
    ax = *(word *)&g_memByte[523144];       // mov ax, teamStarting
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[523146];       // mov ax, teamPlayingUp
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, ax
    if (flags.zero)
        goto l_upper_team_starting;         // jz short @@upper_team_starting

    A0 = 522940;                            // mov A0, offset bottomTeamData
    *(word *)&D1 = 0;                       // mov word ptr D1, 0
    *(byte *)&D2 = 199;                     // mov byte ptr D2, 0C7h

l_upper_team_starting:;
    *(word *)&g_memByte[523118] = 0;        // mov gameState, ST_PLAYERS_TO_INITIAL_POSITIONS
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, ST_STOPPED
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    *(word *)&g_memByte[523124] = 336;      // mov foulXCoordinate, 336
    *(word *)&g_memByte[523126] = 449;      // mov foulYCoordinate, 449
    ax = D1;                                // mov ax, word ptr D1
    *(word *)&g_memByte[523128] = ax;       // mov cameraDirection, ax
    al = D2;                                // mov al, byte ptr D2
    g_memByte[523130] = al;                 // mov byte ptr playerTurnFlags, al
    eax = A0;                               // mov eax, A0
    *(dword *)&g_memByte[523108] = eax;     // mov lastTeamPlayedBeforeBreak, eax
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0
}
