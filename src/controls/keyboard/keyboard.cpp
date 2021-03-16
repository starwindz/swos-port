#include "keyboard.h"

const DefaultKeySet kPl1DefaultKeys = {
    SDL_SCANCODE_UP, SDL_SCANCODE_DOWN, SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_RCTRL, SDL_SCANCODE_RSHIFT
};

const DefaultKeySet kPl2DefaultKeys = {
    SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_A, SDL_SCANCODE_D, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_GRAVE
};

static KeyConfig m_pl1Config(kPl1DefaultKeys);
static KeyConfig m_pl2Config(kPl2DefaultKeys);

static const char kPl1KeyboardSection[] = "player1Keyboard";
static const char kPl2KeyboardSection[] = "player2Keyboard";

GameControlEvents eventsFromAllKeysets()
{
    return m_pl1Config.events() | m_pl2Config.events();
}

GameControlEvents pl1KeyEvents()
{
    return m_pl1Config.events();
}

GameControlEvents pl2KeyEvents()
{
    return m_pl2Config.events();
}

bool pl1HasScancode(SDL_Scancode scancode)
{
    return m_pl1Config.has(scancode);
}

bool pl2HasScancode(SDL_Scancode scancode)
{
    return m_pl2Config.has(scancode);
}

static KeyConfig& getKeyConfig(PlayerNumber player)
{
    return player == kPlayer1 ? m_pl1Config : m_pl2Config;
}

bool playerHasScancode(PlayerNumber player, SDL_Scancode key)
{
    return getKeyConfig(player).has(key);
}

bool pl1HasBasicBindings()
{
    return m_pl1Config.hasMinimumEvents();
}

bool pl2HasBasicBindings()
{
    return m_pl2Config.hasMinimumEvents();
}

const KeyConfig::KeyBindings& getPlayerKeyBindings(PlayerNumber player)
{
    return getKeyConfig(player).getBindings();
}

const KeyConfig::KeyBinding& getPlayerKeyBindingAt(PlayerNumber player, size_t index)
{
    return getKeyConfig(player).getBindingAt(index);
}

void addPlayerKeyBinding(PlayerNumber player, SDL_Scancode key, GameControlEvents events)
{
    getKeyConfig(player).addBinding(key, events);
}

void updatePlayerKeyBindingEventsAt(PlayerNumber player, size_t index, GameControlEvents events)
{
    getKeyConfig(player).updateBindingEventsAt(index, events);
}

void setDefaultKeyPackForPlayer(PlayerNumber player, const DefaultScancodesPack& scancodes)
{
    getKeyConfig(player).setDefaultKeyPack(scancodes);
}

void deleteKeyBindingAt(PlayerNumber player, size_t index)
{
    auto& config = getKeyConfig(player);
    config.deleteBindingAt(index);
}

void loadKeyboardConfig(const CSimpleIni& ini)
{
    m_pl1Config.load(ini, kPl1KeyboardSection, kPl1DefaultKeys);
    m_pl2Config.load(ini, kPl2KeyboardSection, kPl2DefaultKeys);
}

void saveKeyboardConfig(CSimpleIni& ini)
{
    m_pl1Config.save(ini, kPl1KeyboardSection, kPl1DefaultKeys);
    m_pl2Config.save(ini, kPl2KeyboardSection, kPl2DefaultKeys);
}
