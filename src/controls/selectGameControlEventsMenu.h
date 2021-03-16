#pragma once

#include "gameControlEvents.h"

using SetGameControlEventsFunction = std::function<void(int, GameControlEvents, bool)>;
using GetGameControlEventsFunction = std::function<std::pair<GameControlEvents, bool>(int&, char *, int)>;

std::pair<bool, GameControlEvents> getNewGameControlEvents(const char *title);
void updateGameControlEvents(int index, SetGameControlEventsFunction setFunction, GetGameControlEventsFunction getFunction, bool showInverted = false);
