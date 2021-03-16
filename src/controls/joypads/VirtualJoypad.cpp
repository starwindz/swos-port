#include "VirtualJoypad.h"
#include "JoypadConfig.h"
#include "util.h"

VirtualJoypad::VirtualJoypad(const char *name) : m_name(name)
{
}

const char *VirtualJoypad::name() const
{
    return m_name;
}

SDL_JoystickGUID VirtualJoypad::guid() const
{
    return SDL_JoystickGUID{ 'z', 'k', 'z' };
}

bool VirtualJoypad::guidEqual(const SDL_JoystickGUID& guid) const
{
    return ::guidEqual(guid, this->guid());
}

SDL_JoystickID VirtualJoypad::id() const
{
    return -1;
}

GameControlEvents VirtualJoypad::events() const
{
    return kNoGameEvents;
}

GameControlEvents VirtualJoypad::allEventsMask() const
{
    assert(m_config);
    return m_config->allEventsMask();
}

JoypadElementValueList VirtualJoypad::elementValues() const
{
//TODO
    return JoypadElementValueList();
}

int VirtualJoypad::numButtons() const
{
    return 2;
}
int VirtualJoypad::numHats() const
{
    return 1;
}
int VirtualJoypad::numAxes() const
{
    return 0;
}
int VirtualJoypad::numBalls() const
{
    return 0;
}

uint64_t VirtualJoypad::lastSelected() const
{
    return m_lastSelected;
}

void VirtualJoypad::updateLastSelected(uint64_t time)
{
    m_lastSelected = time;
}
