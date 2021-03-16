#pragma once

#include "gameControlEvents.h"
#include "controls.h"

bool getShortFireAndUpdateFireCounter(bool currentFire, PlayerNumber player = kPlayer1);
int16_t eventsToDirection(GameControlEvents events);
