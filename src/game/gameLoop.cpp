#include "gameLoop.h"
#include "game.h"
#include "gameControls.h"
#include "spinningLogo.h"
#include "playerNameDisplay.h"
#include "gameSprites.h"
#include "gameTime.h"
#include "bench.h"
#include "result.h"
#include "camera.h"
#include "pitch.h"
#include "menus.h"
#include "render.h"
#include "controls.h"
#include "keyBuffer.h"
#include "audio.h"
#include "music.h"
#include "comments.h"
#include "sfx.h"
#include "chants.h"
#include "options.h"
#include "replayExitMenu.h"
#include "util.h"

constexpr Uint32 kMinimumPreMatchScreenLength = 1'200;

static Uint32 m_loadingStartTick;

static void showStadiumScreenAndFadeOutMusic(TeamGame *topTeam, TeamGame *bottomTeam);
static void initGameLoop();
static bool updateTimersHandlePauseAndStats();
static void coreGameUpdate();
static void handleHighlightsAndReplays();
static bool gameEnded(TeamGame *topTeam, TeamGame *bottomTeam);
static void updateFrameAndFadeIfNeeded();
static void insertPauseBeforeTheGameIfNeeded();
static void keepStatisticsOnScreen();
static void loadCrowdChantSampleIfNeeded();
static void initGoalSprites();

void gameLoop(TeamGame *topTeam, TeamGame *bottomTeam)
{
    showStadiumScreenAndFadeOutMusic(topTeam, bottomTeam);

    do {
        initGameLoop();

        // the really main game loop ;)
        while (updateTimersHandlePauseAndStats()) {
            // the code doesn't even get to test the flag unless the replay is started
            loadCrowdChantSampleIfNeeded();

            if (!swos.goIntoHighlightsLoop) {
                if (!swos.replayState) {
                    coreGameUpdate();
                    handleHighlightsAndReplays();
                    updateFrameAndFadeIfNeeded();
                } else {
                    //FadeOutToBlack();
                    loadCrowdChantSampleIfNeeded();
                    SWOS::PlayInstantReplayLoop(); // convert me [only ref.]
                }
            } else {
                //FadeOutToBlack();
                SWOS::PlayHighlightsLoop();
            }
        }
    } while (!gameEnded(topTeam, bottomTeam));
}

void showStadiumScreenAndFadeOutMusic(TeamGame *topTeam, TeamGame *bottomTeam)
{
    swos.playGame = 1;

    SetPitchTypeAndNumber();

    invokeWithSaved68kRegisters([] {
        showMenu(swos.stadiumMenu);
    });

    initMatch(topTeam, bottomTeam, true);

    waitForMusicToFadeOut();
}

static void initGameLoop()
{
    swos.g_muteCommentary = swos.g_commentary ? 0 : -1;

    loadCommentary();
    loadSoundEffects();
    loadIntroChant();

    insertPauseBeforeTheGameIfNeeded();
    //fade out to black

    initGameAudio();
    playCrowdNoise();

    if (swos.playGame)
        playFansChant4lSample();

    flushKeyBuffer();

    initBenchBeforeMatch();

    swos.trainingGameCopy = swos.g_trainingGame;
    swos.gameCanceled = 0;
    swos.instantReplayFlag = 0;
    swos.saveHighlights = 0;
    swos.fadeAndSaveReplay = 0;

    setCameraToInitialPosition();

    swos.skipFade = 0;
    ReadTimerDelta();

    waitForKeyboardAndMouseIdle();

    swos.timerBoolean = 0;
    swos.bumpBigSFrame = -1;

    updateFrameAndFadeIfNeeded();
}

bool updateTimersHandlePauseAndStats()
{
    ReadTimerDelta();
    swos.menuCycleTimer = 0;

    while (swos.paused) {
        SWOS::GetKey();
        MainKeysCheck();
        if (isAnyPlayerFiring())
            swos.paused ^= 1;
    }

    if (swos.showingStats)
        keepStatisticsOnScreen();

    swos.frameCount++;

    if (swos.spaceReplayTimer)
        swos.spaceReplayTimer--;

    SWOS::GetKey(); // remove me
    MainKeysCheck(); // & me

    return swos.playGame != 0;
}

static void coreGameUpdate()
{
    moveCamera();
    drawPitch();
    D0 = -1;
    D4 = getCameraX().raw();
    D5 = getCameraY().raw();
    SWOS::SaveCoordinatesForHighlights(); // fixme!!!
    playEnqueuedSamples();
    updateGameTime();
    initGoalSprites();
    updateTeamControls();
    UpdateBall();
    MovePlayers();
    DrawReferee();
    SetCornerFlagSprite();
    spinBigSLogo();
    //ManageAdvertisements();
    DoGoalKeeperSprites();
    DrawControlledPlayerNumbers();
    MarkPlayer();
    setCurrentPlayerName();
    BookPlayer();
    showAndPositionResult();
    SWOS::DrawAnimatedPatterns(); // remove/re-implement me
    drawSprites();
    updateBench();
    UpdateStatistics();
    D0 = -2;
    SWOS::SaveCoordinatesForHighlights();
}

static void handleHighlightsAndReplays()
{
    if (swos.saveHighlights) {
        SaveHighlight();
        swos.saveHighlights = 0;
    }

    if (swos.fadeAndSaveReplay) {
        FadeAndSaveReplay();
        swos.fadeAndSaveReplay = 0;
    }

    if (swos.instantReplayFlag) {
        HandleInstantReplayStateSwitch();
        swos.instantReplayFlag = 0;
        swos.goalCameraMode = 0;
    }
}

static bool gameEnded(TeamGame *topTeam, TeamGame *bottomTeam)
{
    stopAudio();
    //FadeOutToBlack();
    //reload sw title! (which should have been unloaded)
    SetHilFileHeader();
    matchEnded();

    if (!swos.isGameFriendly)
        return true;

    if (swos.g_trainingGame)
        return true;

    showReplayExitMenuAfterFriendly();

    if (!swos.replaySelected)
        return true;

    swos.team1NumAllowedInjuries = 4;
    swos.team2NumAllowedInjuries = 4;

    swos.playGame = 1;
    SetPitchTypeAndNumber();

    initMatch(swos.topTeamPtr, swos.bottomTeamPtr, false);

    return false;
}

static void updateFrameAndFadeIfNeeded()
{
    updateFrame();
    if (!swos.skipFade) {
        //FadeIn();
        swos.skipFade = -1;
        swos.stoppageTimer = 0;
        swos.lastStoppageTimerValue = 0;
        swos.menuCycleTimer = 0;
    }
}

static void insertPauseBeforeTheGameIfNeeded()
{
    if (!doNotPauseLoadingScreen()) {
        auto diff = SDL_GetTicks() - m_loadingStartTick;
        if (diff < kMinimumPreMatchScreenLength) {
            auto pause = kMinimumPreMatchScreenLength - diff;
            logInfo("Pausing loading screen for %d milliseconds (target: %d)", pause, kMinimumPreMatchScreenLength);
            SDL_Delay(pause);
        }
    }
}

static void keepStatisticsOnScreen()
{
    do {
        SWOS::GetKey();
        MainKeysCheck();
        UpdateStatistics();
        SDL_Delay(100);
    } while (swos.showingStats);
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
    swos.goal1TopSprite.pictureIndex = kTopGoalSprite;
    swos.goal2BottomSprite.pictureIndex = kBottomGoalSprite;
}

void SWOS::ShowStadiumInit_OnEnter()
{
    m_loadingStartTick = SDL_GetTicks();
}
