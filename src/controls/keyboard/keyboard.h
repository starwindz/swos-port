#pragma once

#include "controls.h"
#include "scancodes.h"
#include "gameControlEvents.h"
#include "KeyConfig.h"

GameControlEvents eventsFromAllKeysets();
GameControlEvents pl1KeyEvents();
GameControlEvents pl2KeyEvents();

bool pl1HasScancode(SDL_Scancode scancode);
bool pl2HasScancode(SDL_Scancode scancode);
bool playerHasScancode(PlayerNumber player, SDL_Scancode key);
bool pl1HasBasicBindings();
bool pl2HasBasicBindings();

const KeyConfig::KeyBindings& getPlayerKeyBindings(PlayerNumber player);
const KeyConfig::KeyBinding& getPlayerKeyBindingAt(PlayerNumber player, size_t index);

void addPlayerKeyBinding(PlayerNumber player, SDL_Scancode key, GameControlEvents events);
void updatePlayerKeyBindingEventsAt(PlayerNumber player, size_t index, GameControlEvents events);
void setDefaultKeyPackForPlayer(PlayerNumber player, const DefaultScancodesPack& scancodes);

void deleteKeyBindingAt(PlayerNumber player, size_t index);

void loadKeyboardConfig(const CSimpleIni& ini);
void saveKeyboardConfig(CSimpleIni& ini);
