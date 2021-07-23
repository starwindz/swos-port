#pragma once

#include "gameControlEvents.h"
#include "controls.h"

void updateTeamControls();
GameControlEvents getPlayerEvents(PlayerNumber player);
bool getShortFireAndUpdateFireCounter(bool currentFire, PlayerNumber player = kPlayer1);
int16_t eventsToDirection(GameControlEvents events);
GameControlEvents directionToEvents(int16_t direction);
bool isAnyPlayerFiring();
