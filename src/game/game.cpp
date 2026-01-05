#include "game.h"
#include "gameLoop.h"
#include "windowManager.h"
#include "render.h"
#include "audio.h"
#include "music.h"
#include "chants.h"
#include "comments.h"
#include "sfx.h"
#include "options.h"
#include "controls.h"
#include "amigaMode.h"
#include "gameControls.h"
#include "keyBuffer.h"
#include "dump.h"
#include "util.h"
#include "replays.h"
#include "pitch.h"
#include "bench.h"
#include "updateBench.h"
#include "team.h"
#include "player.h"
#include "sprites.h"
#include "gameSprites.h"
#include "gameTime.h"
#include "ball.h"
#include "playerNameDisplay.h"
#include "spinningLogo.h"
#include "result.h"
#include "stats.h"
#include "random.h"
#include "menus.h"
#include "drawMenu.h"
#include "versusMenu.h"
#include "stadiumMenu.h"

constexpr int kWheelZoomFrames = 6;

static TeamGame m_topTeamSaved;
static TeamGame m_bottomTeamSaved;

static bool m_gamePaused;

static bool m_blockZoom;
static int m_zoomFrames;

static void saveTeams();
static void restoreTeams();
static void cancelGame();
static void initializeIngameTeamsAndStartGame(TeamFile *team1, TeamFile *team2, int minSubs, int maxSubs, int paramD7);
static void processPostGameData(TeamFile *team1, TeamFile *team2, int paramD7);
static void initPlayerCardChance();
static void determineStartingTeamAndTeamPlayingUp();
static void initPitchBallFactors();
static void initGameVariables();
static void startingMatch();

// Initializes everything except the sprite graphics, which are needed for the stadium menu.
void initMatch(TeamGame *topTeam, TeamGame *bottomTeam, bool saveOrRestoreTeams)
{
    saveOrRestoreTeams ? saveTeams() : restoreTeams();

    initMatchSprites(topTeam, bottomTeam);
    initPlayerShotChanceTables();   // to be removed later
    setPitchTypeAndNumber();
    loadPitch();
    //InitAdvertisements

    if (!replayingNow()) {
        swos.topTeamPtr = topTeam;
        swos.bottomTeamPtr = bottomTeam;

        initPlayerCardChance();

        swos.gameRandValue = SWOS::rand();

        A4 = topTeam;
        invokeWithSaved68kRegisters(ApplyTeamTactics);
        A4 = bottomTeam;
        invokeWithSaved68kRegisters(ApplyTeamTactics);

        determineStartingTeamAndTeamPlayingUp();

//arrange pitch type pattern coloring
        initPitchBallFactors();
        initGameVariables();

        initDisplaySprites();
        resetGameTime();
        resetResult(topTeam->teamName, bottomTeam->teamName);
        initStats();
//    InitAnimatedPatterns();
        startingMatch();

        if (!swos.g_trainingGame)
            swos.showFansCounter = 100;

        loadCommentary();
    }

    loadSoundEffects();
    loadIntroChant();

    m_blockZoom = false;
    m_zoomFrames = 0;
}

void initializeIngameTeams(int minSubs, int maxSubs, TeamFile *team1, TeamFile *team2)
{
    swos.gameMinSubstitutes = minSubs;
    swos.gameMaxSubstitutes = maxSubs;
    swos.gameTeam1 = team1;
    swos.gameTeam2 = team2;

    A0 = team1;
    D0 = maxSubs;
    cseg_8F66C();

    A0 = team2;
    D0 = maxSubs;
    cseg_8F66C();

    swos.teamsLoaded = 0;
    swos.poolplyrLoaded = 0;

    swos.team1Computer = 0;
    swos.team2Computer = 0;

    if (team1->teamControls == kComputerTeam)
        swos.team1Computer = -1;
    if (team2->teamControls == kComputerTeam)
        swos.team2Computer = -1;

    D0 = team1->prShirtType;
    D1 = team1->prStripesColor;
    D2 = team1->prBasicColor;
    D3 = team1->prShortsColor;
    D4 = team1->prSocksColor;
    A1 = &swos.topTeamInGame;
    SetTeamSecondaryColors();

    D0 = team2->prShirtType;
    D1 = team2->prStripesColor;
    D2 = team2->prBasicColor;
    D3 = team2->prShortsColor;
    D4 = team2->prSocksColor;
    A1 = &swos.bottomTeamInGame;
    SetTeamSecondaryColors();

    A2 = team1;
    A3 = team2;
    SetInGameTeamsPrimaryColors();

    if (swos.g_gameType != kGameTypeCareer && swos.g_allPlayerTeamsEqual) {
        GetAveragePlayerPriceInSelectedTeams();
        swos.averagePlayerPrice = D0;
    }

    A2 = team1;
    A4 = &swos.topTeamInGame;
    A6 = &swos.team1AppPercent;
    InitInGameTeamStructure();

    A2 = team2;
    A4 = &swos.bottomTeamInGame;
    A6 = &swos.team2AppPercent;
    InitInGameTeamStructure();

    if (swos.isGameFriendly) {
        swos.team1NumAllowedInjuries = 4;
        swos.team2NumAllowedInjuries = 4;
    } else {
        int maxInjuries = swos.g_gameType == kGameTypeCareer ? 4 : 2;

        D0 = -1;
        D1 = 0;
        A0 = team1;
        GetNumberOfAvailablePlayers();

        swos.team1NumAllowedInjuries = std::max(0, maxInjuries - D6.asWord());

        D0 = -1;
        D1 = 0;
        A0 = team2;
        GetNumberOfAvailablePlayers();

        swos.team2NumAllowedInjuries = std::max(0, maxInjuries - D6.asWord());
    }

    if (showPreMatchMenus()) {
        showVersusMenu(&swos.topTeamInGame, &swos.bottomTeamInGame, swos.gameName, swos.gameRound, []() {
            loadStadiumSprites(&swos.topTeamInGame, &swos.bottomTeamInGame);
        });
    }

    if (!swos.isGameFriendly && swos.g_gameType != kGameTypeDiyCompetition)
        swos.gameLengthInGame = 0;
    else
        swos.gameLengthInGame = swos.g_gameLength;
}

void matchEnded()
{
    finishCurrentReplay();
}

void startMainGameLoop()
{
    startFadingOutMusic();

    initGameControls();
    initNewReplay();
    updateCursor(true);

    gameLoop(&swos.topTeamInGame, &swos.bottomTeamInGame);

    updateCursor(false);
}

void checkGlobalKeyboardShortcuts(SDL_Scancode scancode, bool pressed)
{
    static SDL_Scancode lastScancode;

    switch (scancode) {
    case SDL_SCANCODE_F1:
        // preserve alt-F1, ultra fast exit from SWOS (actually meant for invoking the debugger ;))
        if (pressed && (SDL_GetModState() & KMOD_ALT)) {
            logInfo("Shutting down via keyboard shortcut...");
            std::exit(EXIT_SUCCESS);
        }
        break;
    case SDL_SCANCODE_F2:
        if (pressed && scancode != lastScancode)
            makeScreenshot();
        break;
    case SDL_SCANCODE_RETURN:
    case SDL_SCANCODE_KP_ENTER:
        {
            auto mod = SDL_GetModState();
            if (pressed && (mod & KMOD_ALT))
                (mod & KMOD_SHIFT) ? toggleFullScreenMode() : toggleBorderlessMaximizedMode();

            updateCursor(isMatchRunning());
        }
        break;
    }

    lastScancode = pressed ? scancode : SDL_SCANCODE_UNKNOWN;
}

bool checkGameKeys()
{
    bool zoomChanged = checkZoomKeys();

    auto key = getKey();

    if (key == SDL_SCANCODE_UNKNOWN || testForPlayerKeys(key))
        return zoomChanged;

    if (key == SDL_SCANCODE_D) {
        toggleDebugOutput();
        return zoomChanged;
    }

    if (isGamePaused()) {
        if (key == SDL_SCANCODE_P && !inBench())
            m_gamePaused = false;
    } else if (showingUserRequestedStats()) {
        if (key == SDL_SCANCODE_S)
            hideStats();
    } else if (!replayingNow()) {
        switch (key) {
        case SDL_SCANCODE_P:
            if (!inBench())
                togglePause();
            break;
        case SDL_SCANCODE_H:
            if (swos.gameState == GameState::kResultAfterTheGame)
                requestFadeAndReplayHighlights();
            break;
        case SDL_SCANCODE_R:
            if (!inBench() && swos.gameState != GameState::kResultAfterTheGame &&
                swos.gameState != GameState::kResultOnHalftime)
                requestFadeAndInstantReplay();
            break;
        case SDL_SCANCODE_S:
            if (swos.gameStatePl != GameState::kInProgress && !inBench() &&
                (swos.gameState < GameState::kStartingGame || swos.gameState > GameState::kGameEnded))
                toggleStats();
            break;
        case SDL_SCANCODE_SPACE:
            {
                constexpr Uint32 kSaveReplayRequestCooldown = 1'500;
                static Uint32 lastSaveRequestTime;
                auto now = SDL_GetTicks();
                if (now - lastSaveRequestTime > kSaveReplayRequestCooldown) {
                    requestFadeAndSaveReplay();
                    lastSaveRequestTime = now;
                }
            }
            break;
        case SDL_SCANCODE_F8:
            toggleMuteCommentary();
            break;
        case SDL_SCANCODE_F9:
            enableSpinningLogo(!spinningLogoEnabled());
            break;
        case SDL_SCANCODE_F10:
            {
                bool chantsEnabled = areCrowdChantsEnabled();
                setCrowdChantsEnabled(!chantsEnabled);
            }
            break;
        case SDL_SCANCODE_PAGEUP:
            if (swos.bottomTeamData.playerNumber || swos.bottomTeamData.playerCoachNumber)
                requestBench2();
            break;
        case SDL_SCANCODE_PAGEDOWN:
            if (swos.topTeamData.playerNumber || swos.topTeamData.playerCoachNumber)
                requestBench1();
            break;
        case SDL_SCANCODE_ESCAPE:
            cancelGame();
            break;
        }
    }

    return zoomChanged;
}

bool checkZoomKeys()
{
    constexpr auto kZoomStep = .25f;

    int wheelDirection = mouseWheelAmount();
    if (wheelDirection)
        m_zoomFrames = wheelDirection > 0 ? kWheelZoomFrames : -kWheelZoomFrames;

    auto keys = SDL_GetKeyboardState(nullptr);
    bool controlHeld = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    bool shiftHeld = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

    bool zoomInRequested = keys[SDL_SCANCODE_KP_PLUS] || m_zoomFrames > 0;
    bool zoomOutRequested = keys[SDL_SCANCODE_KP_MINUS] || m_zoomFrames < 0;

    bool zoomKeysHeld = zoomInRequested || zoomOutRequested;
    bool resetToDefault = keys[SDL_SCANCODE_KP_MULTIPLY] != 0;

    if (resetToDefault) {
        return resetZoom();
    } else if ((controlHeld || shiftHeld) && zoomKeysHeld) {
        if (m_blockZoom)
            return false;
        m_blockZoom = true;
    } else {
        m_blockZoom = false;
    }

    if (m_zoomFrames)
        m_zoomFrames > 0 ? m_zoomFrames-- : m_zoomFrames++;

    auto step = controlHeld ? kZoomStep : 0;

    if (zoomInRequested)
        return zoomIn(step);
    else if (zoomOutRequested)
        return zoomOut(step);
    else
        return false;
}

void updateCursor(bool matchRunning)
{
    bool fullScreen = getWindowMode() != kModeWindow;
    bool disableCursor = matchRunning && fullScreen && !gotMousePlayer();
    SDL_ShowCursor(disableCursor ? SDL_DISABLE : SDL_ENABLE);
}

bool isGamePaused()
{
    return m_gamePaused;
}

void pauseTheGame()
{
    m_gamePaused = true;
}

void togglePause()
{
    m_gamePaused = !m_gamePaused;
}

using namespace SwosVM;

void initTeamsData()
{
    *(dword *)&g_memByte[522748] = 0;       // mov currentScorer, 0
    *(dword *)&g_memByte[522756] = 0;       // mov lastPlayerBeforeGoalkeeper, 0
    *(word *)&g_memByte[457524] = 0;        // mov goalScored, 0
    *(word *)&g_memByte[457526] = 0;        // mov runSlower, 0
    *(word *)&g_memByte[523638] = 0;        // mov whichCard, 0
    *(dword *)&g_memByte[523640] = 0;       // mov bookedPlayer, 0
    *(word *)&g_memByte[523085] = 0;        // mov playerHadBall, 0
    *(dword *)&g_memByte[523087] = 0;       // mov lastKeeperPlayed, 0
    *(dword *)&g_memByte[523092] = 0;       // mov lastTeamPlayed, 0
    *(dword *)&g_memByte[523096] = 0;       // mov lastPlayerPlayed, 0
    *(word *)&g_memByte[523100] = 0;        // mov penalty, 0
    *(word *)&g_memByte[455982] = 0;        // mov goalCameraMode, 0
    *(word *)&g_memByte[523102] = 0;        // mov goalOut, 0
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    *(word *)&g_memByte[523106] = 0;        // mov fireBlocked, 0
    *(dword *)&g_memByte[523108] = 0;       // mov lastTeamPlayedBeforeBreak, 0
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    *(word *)&g_memByte[523122] = 0;        // mov stoppageEventTimer, 0
    *(word *)&g_memByte[523132] = 0;        // mov inGameCounter, 0
    *(word *)&g_memByte[523116] = 100;      // mov gameStatePl, 100
    *(word *)&g_memByte[523118] = 100;      // mov gameState, ST_GAME_IN_PROGRESS
    *(word *)&g_memByte[523134] = 0;        // mov breakState, 0
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&D7 = 1;                       // mov word ptr D7, 1
    ax = *(word *)&g_memByte[449278];       // mov ax, topTeamPlayerNo
    *(word *)&D6 = ax;                      // mov word ptr D6, ax
    ax = *(word *)&g_memByte[449274];       // mov ax, topTeamCoachNo
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    ax = *(word *)&g_memByte[449270];       // mov ax, pl1Coach
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    A0 = 522792;                            // mov A0, offset topTeamData
    A1 = 522940;                            // mov A1, offset bottomTeamData
    A2 = 330096;                            // mov A2, offset team1SpritesTable
    eax = *(dword *)&g_memByte[449724];     // mov eax, topTeamPtr
    A3 = eax;                               // mov A3, eax
    A4 = 522760;                            // mov A4, offset team1StatsData
    A5 = 449298;                            // mov A5, offset pl1Tactics
    *(word *)&D5 = 1;                       // mov word ptr D5, 1
    {
        word src = *(word *)&g_memByte[523146];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp teamPlayingUp, 1
    if (flags.zero)
        goto l_init_teams;                  // jz short @@init_teams

    eax = A0;                               // mov eax, A0
    {
        dword tmp = A1;
        A1 = eax;
        eax = tmp;
    }                                       // xchg eax, A1
    A0 = eax;                               // mov A0, eax

l_init_teams:;
    eax = A1;                               // mov eax, A1
    esi = A0;                               // mov esi, A0
    writeMemory(esi, 4, eax);               // mov [esi+TeamGeneralInfo.opponentsTeam], eax
    eax = A3;                               // mov eax, A3
    writeMemory(esi + 10, 4, eax);          // mov [esi+TeamGeneralInfo.inGameTeamPtr], eax
    eax = A4;                               // mov eax, A4
    writeMemory(esi + 14, 4, eax);          // mov [esi+TeamGeneralInfo.teamStatsPtr], eax
    ax = D6;                                // mov ax, word ptr D6
    writeMemory(esi + 4, 2, ax);            // mov [esi+TeamGeneralInfo.playerNumber], ax
    ax = D2;                                // mov ax, word ptr D2
    writeMemory(esi + 6, 2, ax);            // mov [esi+TeamGeneralInfo.playerCoachNumber], ax
    ax = D1;                                // mov ax, word ptr D1
    writeMemory(esi + 8, 2, ax);            // mov [esi+TeamGeneralInfo.isPlCoach], ax
    eax = A2;                               // mov eax, A2
    writeMemory(esi + 20, 4, eax);          // mov [esi+TeamGeneralInfo.spritesTable], eax
    ax = D5;                                // mov ax, word ptr D5
    writeMemory(esi + 18, 2, ax);           // mov [esi+TeamGeneralInfo.teamNumber], ax
    esi = A5;                               // mov esi, A5
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    esi = A0;                               // mov esi, A0
    writeMemory(esi + 28, 2, ax);           // mov [esi+TeamGeneralInfo.tactics], ax
    writeMemory(esi + 140, 2, 0);           // mov [esi+TeamGeneralInfo.goalkeeperPlaying], 0
    writeMemory(esi + 142, 2, 0);           // mov [esi+TeamGeneralInfo.resetControls], 0
    writeMemory(esi + 30, 2, 10);           // mov [esi+TeamGeneralInfo.updatePlayerIndex], 10
    writeMemory(esi + 32, 4, 0);            // mov [esi+TeamGeneralInfo.controlledPlayer], 0
    writeMemory(esi + 36, 4, 0);            // mov [esi+TeamGeneralInfo.passToPlayerPtr], 0
    writeMemory(esi + 104, 4, 0);           // mov [esi+TeamGeneralInfo.passingKickingPlayer], 0
    writeMemory(esi + 40, 2, 0);            // mov [esi+TeamGeneralInfo.playerHasBall], 0
    writeMemory(esi + 80, 2, 0);            // mov [esi+TeamGeneralInfo.goalkeeperDivingRight], 0
    writeMemory(esi + 82, 2, 0);            // mov [esi+TeamGeneralInfo.goalkeeperDivingLeft], 0
    writeMemory(esi + 84, 2, 0);            // mov [esi+TeamGeneralInfo.ballOutOfPlayOrKeeper], 0
    writeMemory(esi + 86, 2, 0);            // mov [esi+TeamGeneralInfo.goaliePlayingOrOut], 0
    writeMemory(esi + 88, 2, 0);            // mov [esi+TeamGeneralInfo.passingBall], 0
    writeMemory(esi + 90, 2, 0);            // mov [esi+TeamGeneralInfo.passingToPlayer], 0
    writeMemory(esi + 92, 2, 0);            // mov [esi+TeamGeneralInfo.playerSwitchTimer], 0
    writeMemory(esi + 94, 2, 0);            // mov [esi+TeamGeneralInfo.ballInPlay], 0
    writeMemory(esi + 96, 2, 0);            // mov [esi+TeamGeneralInfo.ballOutOfPlay], 0
    writeMemory(esi + 102, 2, 0);           // mov [esi+TeamGeneralInfo.passKickTimer], 0
    writeMemory(esi + 110, 2, 0);           // mov [esi+TeamGeneralInfo.ballCanBeControlled], 0
    writeMemory(esi + 114, 2, 0);           // mov [esi+TeamGeneralInfo.field_72], 0
    writeMemory(esi + 112, 2, -1);          // mov [esi+TeamGeneralInfo.ballControllingPlayerDirection], -1
    writeMemory(esi + 116, 2, 0);           // mov [esi+TeamGeneralInfo.field_74], 0
    writeMemory(esi + 138, 2, 0);           // mov [esi+TeamGeneralInfo.wonTheBallTimer], 0
    writeMemory(esi + 118, 2, -1);          // mov [esi+TeamGeneralInfo.spinTimer], -1
    writeMemory(esi + 60, 1, 0);            // mov [esi+TeamGeneralInfo.field_3C], 0
    writeMemory(esi + 76, 2, 0);            // mov [esi+TeamGeneralInfo.goalkeeperSavedCommentTimer], 0
    writeMemory(esi + 72, 4, 0);            // mov [esi+TeamGeneralInfo.lastHeadingTacklingPlayer], 0
    writeMemory(esi + 78, 2, 0);            // mov [esi+TeamGeneralInfo.field_4E], 0
    writeMemory(esi + 52, 2, 0);            // mov [esi+TeamGeneralInfo.headerOrTackle], 0
    writeMemory(esi + 58, 2, 0);            // mov [esi+TeamGeneralInfo.shooting], 0
    writeMemory(esi + 128, 2, 0);           // mov [esi+TeamGeneralInfo.passInProgress], 0
    ax = *(word *)&g_memByte[449280];       // mov ax, bottomTeamPlayerNo
    *(word *)&D6 = ax;                      // mov word ptr D6, ax
    ax = *(word *)&g_memByte[449276];       // mov ax, bottomTeamCoachNo
    *(word *)&D2 = ax;                      // mov word ptr D2, ax
    ax = *(word *)&g_memByte[449272];       // mov ax, pl2Coach
    *(word *)&D1 = ax;                      // mov word ptr D1, ax
    A0 = 522940;                            // mov A0, offset bottomTeamData
    A1 = 522792;                            // mov A1, offset topTeamData
    A2 = 330140;                            // mov A2, offset team2SpritesTable
    eax = *(dword *)&g_memByte[449728];     // mov eax, bottomTeamPtr
    A3 = eax;                               // mov A3, eax
    A4 = 522776;                            // mov A4, offset team2StatsData
    A5 = 449300;                            // mov A5, offset pl2Tactics
    *(word *)&D5 = 2;                       // mov word ptr D5, 2
    {
        word src = *(word *)&g_memByte[523146];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp teamPlayingUp, 1
    if (flags.zero)
        goto l_next_team;                   // jz short @@next_team

    eax = A0;                               // mov eax, A0
    {
        dword tmp = A1;
        A1 = eax;
        eax = tmp;
    }                                       // xchg eax, A1
    A0 = eax;                               // mov A0, eax

l_next_team:;
    (*(int16_t *)&D7)--;
    flags.sign = (*(int16_t *)&D7 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D7 == 0;      // dec word ptr D7
    if (!flags.sign)
        goto l_init_teams;                  // jns @@init_teams
}

void initPlayersBeforeEnteringPitch()
{
    SWOS::RemoveReferee();                  // call RemoveReferee
    A0 = &swos.team1SpritesTable;           // mov A0, offset team1SpritesTable
    {
        word src = swos.teamPlayingUp;
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp teamPlayingUp, 1
    if (flags.zero)
        goto l_first_team_up;               // jz short @@first_team_up

    A0 = 330140;                            // mov A0, offset team2SpritesTable

l_first_team_up:;
    A1 = 524064;                            // mov A1, offset kTeamsStartingCoordinates
    *(word *)&D7 = 1;                       // mov word ptr D7, 1

l_next_team:;
    *(word *)&D1 = 10;                      // mov word ptr D1, 10

l_next_player:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi, 4);               // mov eax, [esi]
    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 4;
        dword res = dstSigned + srcSigned;
        A0 = res;
    }                                       // add A0, 4
    A2 = eax;                               // mov A2, eax
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    esi = A2;                               // mov esi, A2
    writeMemory(esi + 32, 2, ax);           // mov word ptr [esi+(Sprite.x+2)], ax
    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 591;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 32, 2, src);
    }                                       // add word ptr [esi+(Sprite.x+2)], 591
    esi = A1;                               // mov esi, A1
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    {
        int32_t dstSigned = A1;
        int32_t srcSigned = 2;
        dword res = dstSigned + srcSigned;
        A1 = res;
    }                                       // add A1, 2
    esi = A2;                               // mov esi, A2
    writeMemory(esi + 36, 2, ax);           // mov word ptr [esi+(Sprite.y+2)], ax
    {
        word src = (word)readMemory(esi + 36, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 449;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi + 36, 2, src);
    }                                       // add word ptr [esi+(Sprite.y+2)], 449
    SWOS::Rand();                           // call Rand
    {
        word res = *(word *)&D0 & 7;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 7
    ax = D0;                                // mov ax, word ptr D0
    esi = A2;                               // mov esi, A2
    {
        word src = (word)readMemory(esi + 32, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        src = res;
        writeMemory(esi + 32, 2, src);
    }                                       // add word ptr [esi+(Sprite.x+2)], ax
    ax = (word)readMemory(esi + 32, 2);     // mov ax, word ptr [esi+(Sprite.x+2)]
    writeMemory(esi + 58, 2, ax);           // mov [esi+Sprite.destX], ax
    ax = (word)readMemory(esi + 36, 2);     // mov ax, word ptr [esi+(Sprite.y+2)]
    writeMemory(esi + 60, 2, ax);           // mov [esi+Sprite.destY], ax
    writeMemory(esi + 40, 2, 0);            // mov word ptr [esi+(Sprite.z+2)], 0
    writeMemory(esi + 44, 2, 0);            // mov [esi+Sprite.speed], 0
    writeMemory(esi + 12, 1, 0);            // mov [esi+Sprite.playerState], PL_NORMAL
    writeMemory(esi + 13, 1, 0);            // mov [esi+Sprite.playerDownTimer], 0
    writeMemory(esi + 22, 2, -1);           // mov [esi+Sprite.frameIndex], -1
    writeMemory(esi + 26, 2, 1);            // mov [esi+Sprite.cycleFramesTimer], 1
    writeMemory(esi + 70, 2, -1);           // mov [esi+Sprite.imageIndex], -1
    writeMemory(esi + 42, 2, 0);            // mov [esi+Sprite.direction], 0
    writeMemory(esi + 84, 2, 1);            // mov [esi+Sprite.onScreen], 1
    {
        word src = (word)swos.gameState;
        int16_t dstSigned = src;
        int16_t srcSigned = 21;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp gameState, ST_STARTING_GAME
    if (!flags.zero)
        goto l_halftime;                    // jnz short @@halftime

    writeMemory(esi + 108, 2, 0);           // mov [esi+Sprite.sentAway], 0
    writeMemory(esi + 102, 2, 0);           // mov [esi+Sprite.cards], 0
    writeMemory(esi + 104, 2, 0);           // mov [esi+Sprite.injuryLevel], 0

l_halftime:;
    push(A0);                               // push A0
    push(A1);                               // push A1
    eax = A2;                               // mov eax, A2
    A1 = eax;                               // mov A1, eax
    A0 = 453234;                            // mov A0, offset playerNormalStandingAnimTable
    SetPlayerAnimationTable();              // call SetPlayerAnimationTable
    pop(A1);                                // pop A1
    pop(A0);                                // pop A0
    (*(int16_t *)&D1)--;
    flags.sign = (*(int16_t *)&D1 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D1 == 0;      // dec word ptr D1
    if (!flags.sign)
        goto l_next_player;                 // jns @@next_player

    A0 = 330096;                            // mov A0, offset team1SpritesTable
    {
        word src = swos.teamPlayingUp;
        int16_t dstSigned = src;
        int16_t srcSigned = 2;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp teamPlayingUp, 2
    if (flags.zero)
        goto l_second_team_up;              // jz short @@second_team_up

    A0 = 330140;                            // mov A0, offset team2SpritesTable

l_second_team_up:;
    (*(int16_t *)&D7)--;
    flags.sign = (*(int16_t *)&D7 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D7 == 0;      // dec word ptr D7
    if (!flags.sign)
        goto l_next_team;                   // jns @@next_team
}

void playersLeavingPitch()
{
    *(word *)&g_memByte[449800] = 0;        // mov hideBall, 0
    *(word *)&g_memByte[523122] = 275;      // mov stoppageEventTimer, 275
    *(word *)&g_memByte[523118] = 24;       // mov gameState, ST_PLAYERS_GOING_TO_SHOWER
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

void startPenalties()
{
    *(word *)&g_memByte[449310] = -1;       // mov penaltiesState, -1
    ax = *(word *)&g_memByte[336636];       // mov ax, statsTeam1Goals
    *(word *)&g_memByte[336628] = ax;       // mov savedTeam1Goals, ax
    ax = *(word *)&g_memByte[336638];       // mov ax, statsTeam2Goals
    *(word *)&g_memByte[336630] = ax;       // mov savedTeam2Goals, ax
    *(word *)&g_memByte[336636] = 0;        // mov statsTeam1Goals, 0
    *(word *)&g_memByte[336648] = 0;        // mov team1GoalsDigit1, 0
    *(word *)&g_memByte[336644] = 0;        // mov team1GoalsDigit2, 0
    *(word *)&g_memByte[336638] = 0;        // mov statsTeam2Goals, 0
    *(word *)&g_memByte[336650] = 0;        // mov team2GoalsDigit1, 0
    *(word *)&g_memByte[336646] = 0;        // mov team2GoalsDigit2, 0
    *(word *)&g_memByte[449316] = 0;        // mov team1PenaltyGoals, 0
    *(word *)&g_memByte[449318] = 0;        // mov team2PenaltyGoals, 0
    SWOS::Rand();                           // call Rand
    {
        word res = *(word *)&D0 & 1;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    ax = D0;                                // mov ax, word ptr D0
    *(word *)&g_memByte[523146] = ax;       // mov teamPlayingUp, ax
    SWOS::Rand();                           // call Rand
    {
        word res = *(word *)&D0 & 1;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    ax = D0;                                // mov ax, word ptr D0
    *(word *)&g_memByte[523144] = ax;       // mov teamStarting, ax
    *(word *)&g_memByte[523158] = 11;       // mov team1PenaltyShooterIndex, 11
    *(word *)&g_memByte[523160] = 11;       // mov team2PenaltyShooterIndex, 11
    *(word *)&g_memByte[523152] = 0;        // mov team1PenaltyAttempts, 0
    *(word *)&g_memByte[523154] = 0;        // mov team2PenaltyAttempts, 0
    *(word *)&g_memByte[523148] = 1;        // mov playingPenalties, 1
    *(word *)&g_memByte[523150] = 1;        // mov dontShowScorers, 1
    A0 = 330096;                            // mov A0, offset team1SpritesTable
    *(word *)&D0 = 10;                      // mov word ptr D0, 10

l_team2_sprites_loop:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi, 4);               // mov eax, [esi]
    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 4;
        dword res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        A0 = res;
    }                                       // add A0, 4
    A5 = eax;                               // mov A5, eax
    esi = A5;                               // mov esi, A5
    writeMemory(esi + 102, 2, 0);           // mov [esi+Sprite.cards], 0
    writeMemory(esi + 108, 2, 0);           // mov [esi+Sprite.sentAway], 0
    (*(int16_t *)&D0)--;
    flags.sign = (*(int16_t *)&D0 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D0 == 0;      // dec word ptr D0
    if (!flags.sign)
        goto l_team2_sprites_loop;          // jns short @@team2_sprites_loop

    A0 = 330140;                            // mov A0, offset team2SpritesTable
    *(word *)&D0 = 10;                      // mov word ptr D0, 10

l_team1_sprites_loop:;
    esi = A0;                               // mov esi, A0
    eax = readMemory(esi, 4);               // mov eax, [esi]
    {
        int32_t dstSigned = A0;
        int32_t srcSigned = 4;
        dword res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint32_t>(dstSigned);
        A0 = res;
    }                                       // add A0, 4
    A5 = eax;                               // mov A5, eax
    esi = A5;                               // mov esi, A5
    writeMemory(esi + 102, 2, 0);           // mov [esi+Sprite.cards], 0
    writeMemory(esi + 108, 2, 0);           // mov [esi+Sprite.sentAway], 0
    (*(int16_t *)&D0)--;
    flags.sign = (*(int16_t *)&D0 & 0x8000) != 0;
    flags.zero = *(int16_t *)&D0 == 0;      // dec word ptr D0
    if (!flags.sign)
        goto l_team1_sprites_loop;          // jns short @@team1_sprites_loop

    nextPenalty();                          // call NextPenalty
}

// Next penalty is about to be shot. Check if penalties are over. If not,
// initialize globals for penalty, set penaltyShooterSprite.
// (penalties after the game)
//
void nextPenalty()
{
    ax = *(word *)&g_memByte[523152];       // mov ax, team1PenaltyAttempts
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[523154];       // mov ax, team2PenaltyAttempts
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, ax
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 10;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, 10
    if (flags.carry)
        goto l_still_under_five_attempts;   // jb short @@still_under_five_attempts

    ax = *(word *)&g_memByte[523152];       // mov ax, team1PenaltyAttempts
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[523154];       // mov ax, team2PenaltyAttempts
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, ax
    if (!flags.zero)
        goto l_next_penalty;                // jnz @@next_penalty

    ax = *(word *)&g_memByte[449316];       // mov ax, team1PenaltyGoals
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    ax = *(word *)&g_memByte[449318];       // mov ax, team2PenaltyGoals
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, ax
    if (flags.zero)
        goto l_next_penalty;                // jz @@next_penalty

l_finish_penalties:;
    *(word *)&g_memByte[523148] = 0;        // mov playingPenalties, 0
    ax = *(word *)&g_memByte[336628];       // mov ax, savedTeam1Goals
    *(word *)&g_memByte[336636] = ax;       // mov statsTeam1Goals, ax
    ax = *(word *)&g_memByte[336630];       // mov ax, savedTeam2Goals
    *(word *)&g_memByte[336638] = ax;       // mov statsTeam2Goals, ax
    playersLeavingPitch();                  // call PlayersLeavingPitch
    return;                                 // retn

l_still_under_five_attempts:;
    *(word *)&D0 = 5;                       // mov word ptr D0, 5
    ax = *(word *)&g_memByte[523152];       // mov ax, team1PenaltyAttempts
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    ax = *(word *)&g_memByte[449316];       // mov ax, team1PenaltyGoals
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, ax
    ax = *(word *)&g_memByte[449318];       // mov ax, team2PenaltyGoals
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, ax
    if (flags.carry)
        goto l_finish_penalties;            // jb short @@finish_penalties

    *(word *)&D0 = 5;                       // mov word ptr D0, 5
    ax = *(word *)&g_memByte[523154];       // mov ax, team2PenaltyAttempts
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        *(word *)&D0 = res;
    }                                       // sub word ptr D0, ax
    ax = *(word *)&g_memByte[449318];       // mov ax, team2PenaltyGoals
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned + srcSigned;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, ax
    ax = *(word *)&g_memByte[449316];       // mov ax, team1PenaltyGoals
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = ax;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp word ptr D0, ax
    if (flags.carry)
        goto l_finish_penalties;            // jb @@finish_penalties

l_next_penalty:;
    *(word *)&g_memByte[329028] = 0;        // mov word ptr ballSprite.z+2, 0
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
    *(word *)&g_memByte[449800] = 0;        // mov hideBall, 0
    initTeamsData();                        // call InitTeamsData
    *(word *)&g_memByte[523118] = 31;       // mov gameState, ST_PENALTIES
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&g_memByte[523128] = 0;        // mov cameraDirection, 0
    g_memByte[523130] = 131;                // mov byte ptr playerTurnFlags, 83h
    *(word *)&g_memByte[523124] = 336;      // mov foulXCoordinate, 336
    *(word *)&g_memByte[523126] = 187;      // mov foulYCoordinate, 187
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, 101
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    A6 = 522940;                            // mov A6, offset bottomTeamData
    eax = A6;                               // mov eax, A6
    *(dword *)&g_memByte[523108] = eax;     // mov lastTeamPlayedBeforeBreak, eax
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0
    *(word *)&g_memByte[523156] = 0;        // mov penaltiesTimer, 0
    A0 = 523158;                            // mov A0, offset team1PenaltyShooterIndex
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 18, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.teamNumber], 1
    if (flags.zero)
        goto l_team1;                       // jz short @@team1

    A0 = 523160;                            // mov A0, offset team2PenaltyShooterIndex

l_team1:;
    esi = A0;                               // mov esi, A0
    {
        word src = (word)readMemory(esi, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        writeMemory(esi, 2, src);
    }                                       // sub word ptr [esi], 1
    if (!flags.zero)
        goto l_not_goalkeeper;              // jnz short @@not_goalkeeper

    writeMemory(esi, 2, 10);                // mov word ptr [esi], 10

l_not_goalkeeper:;
    esi = A0;                               // mov esi, A0
    ax = (word)readMemory(esi, 2);          // mov ax, [esi]
    *(word *)&D0 = ax;                      // mov word ptr D0, ax
    esi = A6;                               // mov esi, A6
    eax = readMemory(esi + 20, 4);          // mov eax, [esi+TeamGeneralInfo.spritesTable]
    A0 = eax;                               // mov A0, eax
    {
        word res = *(word *)&D0 << 2;
        *(word *)&D0 = res;
    }                                       // shl word ptr D0, 2
    esi = A0;                               // mov esi, A0
    ebx = *(word *)&D0;                     // movzx ebx, word ptr D0
    eax = readMemory(esi + ebx, 4);         // mov eax, [esi+ebx]
    *(dword *)&g_memByte[523162] = eax;     // mov penaltyShooterSprite, eax
    esi = A6;                               // mov esi, A6
    {
        word src = (word)readMemory(esi + 18, 2);
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned - srcSigned;
        flags.carry = static_cast<uint16_t>(dstSigned) < static_cast<uint16_t>(srcSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
    }                                       // cmp [esi+TeamGeneralInfo.teamNumber], 1
    if (!flags.zero)
        goto l_second_team_shoots;          // jnz short @@second_team_shoots

    {
        word src = *(word *)&g_memByte[523152];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[523152] = src;
    }                                       // add team1PenaltyAttempts, 1
    return;                                 // retn

l_second_team_shoots:;
    {
        word src = *(word *)&g_memByte[523154];
        int16_t dstSigned = src;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        src = res;
        *(word *)&g_memByte[523154] = src;
    }                                       // add team2PenaltyAttempts, 1
}

void startFirstExtraTime()
{
    *(word *)&g_memByte[523142] = 1;        // mov halfNumber, 1
    SWOS::Rand();                           // call Rand
    {
        word res = *(word *)&D0 & 1;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    ax = D0;                                // mov ax, word ptr D0
    *(word *)&g_memByte[523146] = ax;       // mov teamPlayingUp, ax
    SWOS::Rand();                           // call Rand
    {
        word res = *(word *)&D0 & 1;
        *(word *)&D0 = res;
    }                                       // and word ptr D0, 1
    {
        int16_t dstSigned = *(word *)&D0;
        int16_t srcSigned = 1;
        word res = dstSigned + srcSigned;
        flags.carry = res < static_cast<uint16_t>(dstSigned);
        flags.sign = (res & 0x8000) != 0;
        flags.zero = res == 0;
        *(word *)&D0 = res;
    }                                       // add word ptr D0, 1
    ax = D0;                                // mov ax, word ptr D0
    *(word *)&g_memByte[523144] = ax;       // mov teamStarting, ax
    *(word *)&g_memByte[449800] = 0;        // mov hideBall, 0
    initTeamsData();                        // call InitTeamsData
    *(word *)&g_memByte[523122] = 110;      // mov stoppageEventTimer, 110
    *(word *)&g_memByte[523118] = 27;       // mov gameState, ST_FIRST_EXTRA_STARTING
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, ST_STOPPED
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    *(word *)&g_memByte[523128] = -1;       // mov cameraDirection, -1
    A0 = 522792;                            // mov A0, offset topTeamData
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
        goto cseg_75E77;                    // jz short cseg_75E77

    A0 = 522940;                            // mov A0, offset bottomTeamData

cseg_75E77:;
    eax = A0;                               // mov eax, A0
    *(dword *)&g_memByte[523108] = eax;     // mov lastTeamPlayedBeforeBreak, eax
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0
}

void endFirstExtraTime()
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
    *(word *)&g_memByte[449800] = 0;        // mov hideBall, 0
    initTeamsData();                        // call InitTeamsData
    *(word *)&g_memByte[523122] = 110;      // mov stoppageEventTimer, 110
    *(word *)&g_memByte[523118] = 28;       // mov gameState, ST_FIRST_EXTRA_ENDED
    *(word *)&g_memByte[523120] = -1;       // mov breakCameraMode, -1
    *(word *)&g_memByte[523116] = 101;      // mov gameStatePl, ST_STOPPED
    *(word *)&g_memByte[523104] = 0;        // mov gameNotInProgressCounterWriteOnly, 0
    *(word *)&g_memByte[523128] = -1;       // mov cameraDirection, -1
    A0 = 522792;                            // mov A0, offset topTeamData
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
        goto cseg_75F45;                    // jz short cseg_75F45

    A0 = 522940;                            // mov A0, offset bottomTeamData

cseg_75F45:;
    eax = A0;                               // mov eax, A0
    *(dword *)&g_memByte[523108] = eax;     // mov lastTeamPlayedBeforeBreak, eax
    *(word *)&g_memByte[523112] = 0;        // mov stoppageTimerTotal, 0
    *(word *)&g_memByte[523114] = 0;        // mov stoppageTimerActive, 0
    stopAllPlayers();                       // call StopAllPlayers
    *(word *)&g_memByte[449796] = 0;        // mov cameraXVelocity, 0
    *(word *)&g_memByte[449798] = 0;        // mov cameraYVelocity, 0
}

void checkIfGoalkeeperClaimedTheBall()
{
    if (swos.gameState == GameState::kKeeperHoldsTheBall) {
        auto team = swos.lastTeamPlayedBeforeBreak;
        A6 = team;
        A1 = team->players[0];
        A2 = &swos.ballSprite;  // the original game fails to set this
        goalkeeperClaimedTheBall();
    } else {
        swos.breakCameraMode = -1;
        swos.gameStatePl = GameState::kStopped;
        swos.stoppageTimerTotal = 0;
        swos.stoppageTimerActive = 0;
        stopAllPlayers();
        swos.cameraXVelocity = 0;
        swos.cameraYVelocity = 0;
    }
}

static void saveTeams()
{
    m_topTeamSaved = swos.topTeamInGame;
    m_bottomTeamSaved = swos.bottomTeamInGame;
}

static void restoreTeams()
{
    swos.topTeamInGame = m_topTeamSaved;
    swos.bottomTeamInGame = m_bottomTeamSaved;
}

static void rigTheScoreForPlayerToLose(int playerNo)
{
    bool team1Player = playerNo == 1;
    auto& loserTotalGoals = team1Player ? swos.team1TotalGoals : swos.team2TotalGoals;
    auto& loserStatGoals = team1Player ? swos.statsTeam1Goals : swos.statsTeam1Goals;
    auto& loserStatGoalsCopy = team1Player ? swos.statsTeam1GoalsCopy : swos.statsTeam1GoalsCopy;
    auto& winnerTotalGoals = team1Player ? swos.team2TotalGoals : swos.team1TotalGoals;
    auto& winnerStatGoals = team1Player ? swos.statsTeam2Goals : swos.statsTeam1Goals;
    auto& winnerStatsGoalsCopy = team1Player ? swos.statsTeam2GoalsCopy : swos.statsTeam1GoalsCopy;

    winnerTotalGoals += 5;
    winnerStatGoals += 5;
    while (loserTotalGoals >= winnerTotalGoals) {
        winnerTotalGoals++;
        winnerStatGoals++;
    }

    D0 = playerNo;
    D5 = winnerTotalGoals - loserTotalGoals;
    AssignFakeGoalsToScorers();

    winnerStatsGoalsCopy = winnerStatGoals;
    loserStatGoalsCopy = loserStatGoals;
}

static void cancelGame()
{
    if (swos.playGame) {
        swos.playGame = 0;
        if (swos.gameState == GameState::kInProgress ||
            swos.gameState != GameState::kPlayersGoingToShower && swos.gameState != GameState::kResultAfterTheGame) {
            swos.stateGoal = 0;
            auto team1 = &swos.topTeamData;
            auto team2 = &swos.bottomTeamData;
            if (swos.topTeamData.teamNumber != 1)
                std::swap(team1, team2);

            bool team1Cpu = !team1->playerNumber && !team1->isPlCoach;
            bool team2Cpu = !team2->playerNumber && !team2->isPlCoach;
            bool playerVsCpu = team1Cpu && !team2Cpu || !team1Cpu && team2Cpu;

            if (playerVsCpu && !gameAtZeroMinute()) {
                int teamNo = team1Cpu ? 2 : 1;
                rigTheScoreForPlayerToLose(teamNo);
            } else {
                swos.gameCanceled = 1;
            }
            swos.extraTimeState = 0;
            swos.penaltiesState = 0;
        }
    }
}

static void initializeIngameTeamsAndStartGame(TeamFile *team1, TeamFile *team2, int minSubs, int maxSubs, int paramD7)
{
    initializeIngameTeams(minSubs, maxSubs, team1, team2);
    saveCurrentMenuAndStartGameLoop();
    processPostGameData(team1, team2, paramD7);

    startMenuSong();
    enqueueMenuFadeIn();
}

static void processPostGameData(TeamFile *team1, TeamFile *team2, int paramD7)
{
    if (!swos.gameCanceled) {
        if (!swos.g_trainingGame) {
            A1 = team1;
            A2 = team2;
            D0 = swos.plg_D3_param;
            cseg_30BD1();

            if (paramD7 < 0) {
                return;
            } else {
                for (const auto teamData : { std::make_pair(team1, &swos.topTeamInGame), std::make_pair(team2, &swos.bottomTeamInGame) }) {
                    auto teamFile = teamData.first;
                    auto teamGame = teamData.second;

                    A1 = teamGame;
                    A2 = teamFile;
                    D7 = paramD7;
                    cseg_2F3AB();

                    A1 = teamGame;
                    A2 = teamFile;
                    UpdatePlayerInjuries();

                    A1 = teamGame;
                    A2 = teamFile;
                    cseg_2F194();

                    A1 = teamGame;
                    A2 = teamFile;
                    cseg_2F0E2();
                }
            }
        } else {
            swos.gameTeam1->andWith0xFE |= 1;
            swos.gameTeam2->andWith0xFE |= 1;

            A1 = &swos.topTeamInGame;
            A2 = team1;
            UpdatePlayerInjuries();

            A1 = &swos.bottomTeamInGame;
            A2 = team2;
            UpdatePlayerInjuries();
        }
    }
}

static void initPlayerCardChance()
{
    assert(swos.gameLengthInGame <= 3);

    // when the foul is made 4 bits (1-4) from currentGameTick are extracted and compared to this value;
    // if greater the player gets a card (yellow or red)
    static const int kPlayerCardChancesPerGameLength[16][4] = {
        4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 9, 10,    // 3 min
        2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 6,     // 5 min
        1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4,     // 7 min
        1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3,     // 10 min
    };

    auto chanceTable = kPlayerCardChancesPerGameLength[swos.gameLengthInGame];

    int chanceIndex = (SWOS::rand() & 0x1e) >> 1;

    swos.playerCardChance = chanceTable[chanceIndex];
}

static void determineStartingTeamAndTeamPlayingUp()
{
    swos.teamPlayingUp = (SWOS::rand() & 1) + 1;
    swos.teamStarting = (SWOS::rand() & 1) + 1;
}

static void initPitchBallFactors()
{
    static const int kPitchBallSpeedInfluence[] = { -3, 4, 1, 0, 0, -1, -1 };
    static const int kPitchBallSpeedInfluenceAmiga[] = { -2, 2, 3, 0, 0, -1, -1 };
    static const int kBallSpeedBounceFactorTable[] = { 24, 80, 80, 72, 64, 40, 32 };
    static const int kBallBounceFactorTable[] = { 88, 112, 104, 104, 96, 88, 80 };

    int pitchType = getPitchType();
    assert(static_cast<size_t>(pitchType) <= 6);

    const auto& pitchBallSpeedInfluence = amigaModeActive() ? kPitchBallSpeedInfluenceAmiga : kPitchBallSpeedInfluence;
    swos.pitchBallSpeedFactor = pitchBallSpeedInfluence[pitchType];
    swos.ballSpeedBounceFactor = kBallSpeedBounceFactorTable[pitchType];
    swos.ballBounceFactor = kBallBounceFactorTable[pitchType];
}

static void initGameVariables()
{
    swos.playingPenalties = 0;
    swos.dontShowScorers = 0;
    swos.statsTimer = 0;
    swos.g_waitForPlayerToGoInTimer = 0;
    swos.g_substituteInProgress = 0;
    swos.statsTeam1Goals = 0;
    swos.team1GoalsDigit1 = 0;
    swos.team1GoalsDigit2 = 0;
    swos.statsTeam2Goals = 0;
    swos.team2GoalsDigit1 = 0;
    swos.team2GoalsDigit2 = 0;

    swos.team1TotalGoals = 0;
    swos.team2TotalGoals = 0;
    if (swos.secondLeg) {
        swos.team1TotalGoals = swos.team1GoalsFirstLeg;
        swos.team2TotalGoals = swos.team2GoalsFirstLeg;
    }

    swos.team1NumSubs = 0;
    swos.team2NumSubs = 0;

    memset(&swos.team1StatsData, 0, sizeof(TeamStatsData));
    memset(&swos.team2StatsData, 0, sizeof(TeamStatsData));

    memset((char *)&swos.topTeamData + 24, 0, sizeof(swos.topTeamData) - 24);
    memset((char *)&swos.bottomTeamData + 24, 0, sizeof(swos.bottomTeamData) - 24);

    swos.goalCounter = 0;
    swos.stateGoal = 0;

    swos.frameCount = 0;
    swos.cameraXVelocity = swos.cameraYVelocity = 0;

    swos.pl1Fire = 0;
    swos.pl2Fire = 0;

    swos.longFireFlag = swos.longFireTime = 0;

    swos.currentGameTick = 0;
    swos.currentTick = 0;

    swos.AI_turnDirection = 1;
}

static void startingMatch()
{
    constexpr int kStartingBallX = 1672, kStartingBallY = 449;
    constexpr int kInitialDelayBeforeKickOff = 100;

    swos.halfNumber = 1;
    swos.hideBall = 0;
    setBallPosition(kStartingBallX, kStartingBallY);

    initTeamsData();    // careful, this function resets some stuff, so keep it up here for now

    swos.stoppageEventTimer = kInitialDelayBeforeKickOff;
    swos.gameState = GameState::kStartingGame;
    swos.gameStatePl = GameState::kStopped;
    swos.lastTeamPlayedBeforeBreak = &swos.topTeamData;
    swos.stoppageTimerTotal = 0;
    swos.stoppageTimerActive = 0;

    swos.breakCameraMode = -1;
    swos.cameraDirection = -1;
    swos.cameraXVelocity = 0;
    swos.cameraYVelocity = 0;

    stopAllPlayers();
    initPlayersBeforeEnteringPitch();
}

// in:
//      D0 = 1
//      D1 = min substitutes
//      D2 = max substitutes
//      D3 = 0
//      D7 = -1
//      A1 -> team 1 (structures from file)
//      A2 -> team 2
//
void SWOS::InitializeInGameTeamsAndStartGame()
{
    swos.plg_D0_param = D0;
    swos.plg_D3_param = D3;

    invokeWithSaved68kRegisters([]() {
        auto topTeamFile = A1.as<TeamFile *>();
        auto bottomTeamFile = A2.as<TeamFile *>();
        int minSubs = D1.asWord();
        int maxSubs = D2.asWord();
        initializeIngameTeamsAndStartGame(topTeamFile, bottomTeamFile, minSubs, maxSubs, D7.asWord());
    });

    SwosVM::ax = swos.gameCanceled;
    SwosVM::flags.zero = !SwosVM::ax;
}

void SWOS::EndProgram()
{
    std::exit(EXIT_FAILURE);
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
