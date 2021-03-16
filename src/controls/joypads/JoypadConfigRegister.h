#pragma once

#include "JoypadGuidConfig.h"

class Joypad;
class JoypadConfig;

class JoypadConfigRegister
{
public:
    JoypadConfig *config(const SDL_JoystickGUID& guid, bool secondary = false);
    void loadConfig(const CSimpleIni& ini);
    void saveConfig(CSimpleIni& ini);

private:
    std::list<JoypadGuidConfig> m_config;
};
