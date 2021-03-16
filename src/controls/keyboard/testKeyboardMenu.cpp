#include "testKeyboardMenu.h"
#include "testControlsMenu.h"
#include "scancodes.h"
#include "keyboard.h"
#include "controls.h"

static PlayerNumber m_player;

static std::vector<std::string> getControls()
{
    std::vector<std::string> keys;

    int totalKeys;
    auto keyboardState = SDL_GetKeyboardState(&totalKeys);

    while (totalKeys--) {
        if (keyboardState[totalKeys])
            keys.emplace_back(scancodeToString(static_cast<SDL_Scancode>(totalKeys)));
    }

    return keys;
}

static GameControlEvents getEvents()
{
    return m_player == kPlayer1 ? pl1KeyEvents() : pl2KeyEvents();
}

void showTestKeyboardMenu(PlayerNumber player)
{
    m_player = player;
    setGlobalShortcutsEnabled(false);

    showTestControlsMenu("TEST KEYBOARD", "KEYS PRESSED:", false, getControls, getEvents);

    setGlobalShortcutsEnabled(true);
}
