#pragma once

#include "gameControlEvents.h"
#include "controls.h"

std::pair<bool, DefaultScancodesPack> promptForDefaultKeys(PlayerNumber player);
SDL_Scancode getControlKey(PlayerNumber player, int warningY);
