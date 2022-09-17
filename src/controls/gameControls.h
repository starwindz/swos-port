#pragma once

#include "gameControlEvents.h"
#include "controls.h"

std::function<void()> updateTeamControls();
GameControlEvents getPlayerEvents(PlayerNumber player);
bool isPlayerFiring(PlayerNumber player);
bool getShortFireAndBumpFireCounter(bool currentFire, PlayerNumber player = kPlayer1);
int16_t eventsToDirection(GameControlEvents events);
GameControlEvents directionToEvents(int16_t direction);
bool isAnyPlayerFiring();
