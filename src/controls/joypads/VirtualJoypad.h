#pragma once

#include "gameControlEvents.h"
#include "JoypadElementValue.h"

class JoypadConfig;

class VirtualJoypad
{
public:
    VirtualJoypad(const char *name);
    const char *name() const;
    SDL_JoystickGUID guid() const;
    bool guidEqual(const SDL_JoystickGUID& guid) const;
    SDL_JoystickID id() const;

    JoypadConfig *config() { return m_config; }

    GameControlEvents events() const;
    GameControlEvents allEventsMask() const;
    JoypadElementValueList elementValues() const;
    bool anyUnassignedButtonDown() const { return false; }
    bool anyButtonDown() const { return false; }

    int numButtons() const;
    int numHats() const;
    int numAxes() const;
    int numBalls() const;

    uint64_t lastSelected() const;
    void updateLastSelected(uint64_t time);

private:
    uint64_t m_lastSelected = 0;
    const char *m_name;
    JoypadConfig *m_config = nullptr;
};
