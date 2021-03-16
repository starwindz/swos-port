#include "mouse.h"

static GameControlEvents m_events;
static GameControlEvents m_oppositeDirectionKick;
static Uint32 m_longKickStart;

GameControlEvents mouseEvents()
{
    return m_events;
}

void resetMouse()
{
    m_events = kNoGameEvents;
    m_longKickStart = 0;
}

static void startLongKick()
{
    m_longKickStart = SDL_GetTicks();
    m_oppositeDirectionKick = kGameEventKick;
    if (m_events & kGameEventLeft)
        m_oppositeDirectionKick |= kGameEventRight;
    if (m_events & kGameEventRight)
        m_oppositeDirectionKick |= kGameEventLeft;
    if (m_events & kGameEventUp)
        m_oppositeDirectionKick |= kGameEventDown;
    if (m_events & kGameEventDown)
        m_oppositeDirectionKick |= kGameEventUp;
}

static bool longKickRunning()
{
    constexpr int kLongKickLength = 80;
    return m_longKickStart + kLongKickLength >= SDL_GetTicks();
}

void updateMouseButtons(Uint8 button, Uint8 state)
{
    if (state == SDL_PRESSED) {
        switch (button) {
        case SDL_BUTTON_LEFT:
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK)
                m_events |= kGameEventBench;
            else
                m_events |= kGameEventKick;
            break;

        case SDL_BUTTON_RIGHT:
            if (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK)
                m_events |= kGameEventBench;
            else
                startLongKick();
            break;

        case SDL_BUTTON_MIDDLE:
            m_events = kNoGameEvents;
            break;
        }
    } else {
        assert(state == SDL_RELEASED);

        switch (button) {
        case SDL_BUTTON_LEFT:
            m_events &= ~(kGameEventKick | kGameEventBench);
            break;

        case SDL_BUTTON_RIGHT:
            m_events &= ~kGameEventBench;
            break;
        }
    }
}

void updateMouseMovement(Sint32 xrel, Sint32 yrel, Uint32 state)
{
    m_events = kNoGameEvents;

    if (state & SDL_BUTTON_MMASK)
        return;

    bool longKickNow = longKickRunning();
    if (longKickNow)
        m_events = m_oppositeDirectionKick;

    if (xrel < 0)
        m_events |= kGameEventLeft;

    if (xrel > 0)
        m_events |= kGameEventRight;

    if (yrel < 0)
        m_events |= kGameEventUp;

    if (yrel > 0)
        m_events |= kGameEventDown;

    if ((state & SDL_BUTTON_LMASK) && (state & SDL_BUTTON_RMASK))
        m_events |= kGameEventBench;
    else if (state & SDL_BUTTON_LMASK)
        m_events |= kGameEventKick;
    else if ((state & SDL_BUTTON_RMASK) && !longKickNow)
        startLongKick();
}
