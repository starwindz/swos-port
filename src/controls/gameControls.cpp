#include "gameControls.h"
#include "keyboard.h"
#include "mouse.h"
#include "joypads.h"

static int m_pl1FireCounter;
static int m_pl2FireCounter;

bool getShortFireAndUpdateFireCounter(bool currentFire, PlayerNumber player /* = kPlayer1 */)
{
    static bool s_lastFired;
    auto& fireCounter = player == kPlayer1 ? m_pl1FireCounter : m_pl2FireCounter;

    bool shortFire = false;

    if (s_lastFired) {
        if (currentFire) {
            if (fireCounter)
                fireCounter--;
        } else {
            s_lastFired = false;
            fireCounter = -fireCounter;
        }
    } else if (currentFire) {
        shortFire = true;
        s_lastFired = true;
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

// We must do this since without it there are problems with doing long kicks (seems the game processes
// event in a way that up always trumps down if they're both active at the same time).
static GameControlEvents filterOverlappedEvents(PlayerNumber player, GameControlEvents events)
{
    static GameControlEvents s_oldPl1Events;
    static GameControlEvents s_oldPl2Events;

    auto& oldEvents = player == kPlayer1 ? s_oldPl1Events : s_oldPl2Events;

    if ((events & kGameEventUp) && (events & kGameEventDown)) {
        if (!(oldEvents & kGameEventUp))
            events &= ~kGameEventDown;
        else
            events &= ~kGameEventUp;
    }
    if ((events & kGameEventLeft) && (events & kGameEventRight)) {
        if (!(oldEvents & kGameEventLeft))
            events &= ~kGameEventRight;
        else
            events &= ~kGameScreenWidth;
    }

    return oldEvents = events;
}

static GameControlEvents getPlayerEvents(PlayerNumber player)
{
    static_assert(kMaxGameEvent == 64, "You missed a spot!");

    assert(player == kPlayer1 || player == kPlayer2);

    auto events = kNoGameEvents;

    auto controls = player == kPlayer1 ? getPl1Controls() : getPl2Controls();

    switch (controls) {
    case kKeyboard1:
        assert(player == kPlayer1);
        events = pl1KeyEvents();
        break;
    case kKeyboard2:
        assert(player == kPlayer2);
        events = pl2KeyEvents();
        break;
    case kMouse:
        events = mouseEvents();
        break;
    case kJoypad:
        events = player == kPlayer1 ? pl1JoypadEvents() : pl2JoypadEvents();
        break;
    default:
        assert(false);
    }

    return filterOverlappedEvents(player, events);
}

struct SwosGameControls
{
    byte& fire;
    byte& shortFire;
    byte& secondaryFire;
    int16_t& fireCounter;
    word& direction;
};

SwosGameControls getGameControls(PlayerNumber player)
{
    if (player == kPlayer1)
        return { swos.pl1Fire, swos.pl1ShortFire, swos.pl1SecondaryFire, swos.pl1FireCounter, swos.pl1Direction };
    else
        return { swos.pl2Fire, swos.pl2ShortFire, swos.pl2SecondaryFire, swos.pl2FireCounter, swos.pl2Direction };
}

static void updateGameControls(PlayerNumber player)
{
    auto events = getPlayerEvents(player);
    const auto& controls = getGameControls(player);

    auto& fireCounter = player == kPlayer1 ? m_pl1FireCounter : m_pl2FireCounter;
    fireCounter = controls.fireCounter;

    controls.fire = -((events & kGameEventKick) != 0);
    controls.secondaryFire = -((events & kGameEventBench) != 0);
    controls.shortFire = getShortFireAndUpdateFireCounter(controls.fire != 0);
    controls.fireCounter = fireCounter;
    controls.direction = eventsToDirection(events);

    if (events & kGameEventPause)
        swos.paused ^= 1;
}

void SWOS::Player1StatusProc()
{
    updateGameControls(kPlayer1);
}

void SWOS::Player2StatusProc()
{
    updateGameControls(kPlayer2);
}
