#include "gameControls.h"
#include "keyboard.h"
#include "mouse.h"
#include "joypads.h"
#include "bench.h"

struct SwosGameControls {
    byte& fire;
    byte& secondaryFire;
    int& fireCounter;
    word& direction;
    byte shortFire;
};

// fire counter starts from -1 and counts downwards while the fire button is pressed
// if 0 don't touch it
// quick fire is activated after 1 and normal fire after 4 frames
// when the fire button is released, the counter is negated (becoming positive)
static int m_pl1FireCounter;
static int m_pl2FireCounter;

static GameControlEvents filterOverlappedEvents(PlayerNumber player, GameControlEvents events);
static SwosGameControls updateGameControls(PlayerNumber player);
static bool handleStatsFireExit();
static void updateTeamControls(TeamGeneralInfo *team, const SwosGameControls& controls);

// Sets control related fields in team structure. Called once per frame.
// Handles one team per frame (next team in next frame).
//
void updateTeamControls()
{
    RunStoppageEventsAndSetAnimationTables();

    processControlEvents();

    if (handleStatsFireExit())
        return;

    static int s_teamSwitchCounter;
    auto team = ++s_teamSwitchCounter & 1 ? &swos.topTeamData : &swos.bottomTeamData;

    A6 = team;
    UpdateControlledPlayer();
    UpdatePlayerBeingPassedTo();

    if (team->playerNumber) {
        auto player = team->playerNumber == 2 ? kPlayer2 : kPlayer1;
        const auto& controls = updateGameControls(player);

        updateTeamControls(team, controls);
    }

    if (!team->resetControls) {
        if (inBench()) {
            team->currentAllowedDirection = -1;
            team->quickFire = 0;
            team->normalFire = 0;
            team->firePressed = 0;
            team->fireThisFrame = 0;
            team->fireCounter = 0;
        }

        UpdatePlayersAndBall();

        if (team->headerOrTackle) {
            team->headerOrTackle = 0;
            auto& fireCounter = team->playerNumber == 2 ? m_pl2FireCounter : m_pl1FireCounter;
            fireCounter = 0;
        }
    }
}

GameControlEvents getPlayerEvents(PlayerNumber player)
{
    assert(player == kPlayer1 || player == kPlayer2);

    auto events = kNoGameEvents;

    auto controls = player == kPlayer1 ? getPl1Controls() : getPl2Controls();

    switch (controls) {
    case kKeyboard1:
        assert(player == kPlayer1);
        events = keyboard1Events();
        break;
    case kKeyboard2:
        assert(player == kPlayer2);
        events = keyboard2Events();
        break;
    case kMouse:
        events = mouseEvents();
        break;
    case kJoypad:
        events = player == kPlayer1 ? pl1JoypadEvents() : pl2JoypadEvents();
        break;
    default:
        return events;
    }

    return filterOverlappedEvents(player, events);
}

// We must do this since without it there are problems with doing long kicks (seems the game processes
// event in a way that up always trumps down if they're both active at the same time).
static GameControlEvents filterOverlappedEvents(PlayerNumber player, GameControlEvents events)
{
    static GameControlEvents s_oldPl1Events;
    static GameControlEvents s_oldPl2Events;
    static GameControlEvents s_pl1LastVertical;
    static GameControlEvents s_pl1LastHorizontal;
    static GameControlEvents s_pl2LastVertical;
    static GameControlEvents s_pl2LastHorizontal;

    auto& oldEvents = player == kPlayer1 ? s_oldPl1Events : s_oldPl2Events;
    auto& forceVertical = player == kPlayer1 ? s_pl1LastVertical : s_pl2LastVertical;
    auto& forceHorizontal = player == kPlayer1 ? s_pl1LastHorizontal : s_pl2LastHorizontal;

    if ((events & kGameEventUp) && (events & kGameEventDown)) {
        events &= ~(kGameEventUp | kGameEventDown);
        if (!forceVertical) {
            if (oldEvents & kGameEventUp)
                forceVertical = kGameEventDown;
            else
                forceVertical = kGameEventUp;
        }
        events |= forceVertical;
    } else {
        forceVertical = kNoGameEvents;
    }

    if ((events & kGameEventLeft) && (events & kGameEventRight)) {
        events &= ~(kGameEventLeft | kGameEventRight);
        if (!forceHorizontal) {
            if (oldEvents & kGameEventLeft)
                forceHorizontal = kGameEventRight;
            else
                forceHorizontal = kGameEventLeft;
        }
        events |= forceHorizontal;
    } else {
        forceHorizontal = kNoGameEvents;
    }

    return oldEvents = events;
}

SwosGameControls getGameControls(PlayerNumber player)
{
    if (player == kPlayer1)
        return { swos.pl1Fire, swos.pl1SecondaryFire, m_pl1FireCounter, swos.pl1Direction };
    else
        return { swos.pl2Fire, swos.pl2SecondaryFire, m_pl2FireCounter, swos.pl2Direction };
}

static SwosGameControls updateGameControls(PlayerNumber player)
{
    static_assert(kMaxGameEvent == 256, "You missed a spot!");

    processControlEvents();

    auto events = getPlayerEvents(player);
    auto controls = getGameControls(player);

    controls.fire = -((events & kGameEventKick) != 0);

    controls.secondaryFire = -((events & kGameEventBench) != 0);
    controls.shortFire = getShortFireAndUpdateFireCounter(controls.fire != 0, player);
    controls.direction = eventsToDirection(events);

    if (events & kGameEventPause)
        swos.paused ^= 1;

    // change when MainKeysCheck() is converted
    if (events & kGameEventReplay) {
        swos.convertedKey = 'R';
        swos.lastKey = sdlScancodeToPc(SDL_SCANCODE_R);
        MainKeysCheck();
    }
    if (events & kGameEventSaveHighlight) {
        swos.convertedKey = ' ';
        swos.lastKey = sdlScancodeToPc(SDL_SCANCODE_SPACE);
        MainKeysCheck();
    }

    return controls;
}

void SWOS::Player1StatusProc()
{
    updateGameControls(kPlayer1);
}

void SWOS::Player2StatusProc()
{
    updateGameControls(kPlayer2);
}

static bool handleStatsFireExit()
{
    if (swos.statsFireExit) {
        if (!((getPlayerEvents(kPlayer1) | getPlayerEvents(kPlayer2)) & kGameEventKick))
            swos.statsFireExit = 0;

        return true;
    }

    return false;
}

void updateTeamControls(TeamGeneralInfo *team, const SwosGameControls& controls)
{
    assert(team->playerNumber);

    team->currentAllowedDirection = controls.direction;
    team->direction = controls.direction;
    team->fireThisFrame = controls.shortFire;
    team->secondaryFire = controls.secondaryFire;

    if (team->firePressed = controls.fire)
        team->fireCounter++;
    else
        team->fireCounter = 0;

    team->quickFire = 0;
    team->normalFire = 0;

    if (controls.fireCounter) {
        if (controls.fireCounter < 0) {
            if (controls.fireCounter < -4) {
                team->normalFire = -1;
                controls.fireCounter = 0;
            }
        } else {
            if (controls.fireCounter > 4)
                team->normalFire = -1;
            else
                team->quickFire = -1;

            controls.fireCounter = 0;
        }
    }
}

bool getShortFireAndUpdateFireCounter(bool currentFire, PlayerNumber player /* = kPlayer1 */)
{
    static bool s_pl1LastFired;
    static bool s_pl2LastFired;

    auto& fireCounter = player == kPlayer1 ? m_pl1FireCounter : m_pl2FireCounter;
    auto& lastFired = player == kPlayer1 ? s_pl1LastFired : s_pl2LastFired;

    bool shortFire = false;

    if (lastFired) {
        if (currentFire) {
            if (fireCounter)
                fireCounter--;
        } else {
            lastFired = false;
            fireCounter = -fireCounter;
        }
    } else if (currentFire) {
        shortFire = true;
        lastFired = true;
        fireCounter = -1;
    }

    return shortFire;
}

int16_t eventsToDirection(GameControlEvents events)
{
    int16_t direction = -1;

    bool left = (events & kGameEventLeft) != 0;
    bool right = (events & kGameEventRight) != 0;
    bool up = (events & kGameEventUp) != 0;
    bool down = (events & kGameEventDown) != 0;

    if (up && right)
        direction = 1;
    else if (down && right)
        direction = 3;
    else if (down && left)
        direction = 5;
    else if (up && left)
        direction = 7;
    else if (up)
        direction = 0;
    else if (right)
        direction = 2;
    else if (down)
        direction = 4;
    else if (left)
        direction = 6;

    return direction;
}

GameControlEvents directionToEvents(int16_t direction)
{
    switch (direction) {
    case 0:
        return kGameEventUp;
    case 1:
        return kGameEventUp | kGameEventRight;
    case 2:
        return kGameEventRight;
    case 3:
        return kGameEventDown | kGameEventRight;
    case 4:
        return kGameEventDown;
    case 5:
        return kGameEventDown | kGameEventLeft;
    case 6:
        return kGameEventLeft;
    case 7:
        return kGameEventUp | kGameEventLeft;
    default:
        assert(false);
    case -1:
        return kNoGameEvents;
    }
}

bool isAnyPlayerFiring()
{
    return ((getPlayerEvents(kPlayer1) | getPlayerEvents(kPlayer2)) & kGameEventKick) != 0;
}
