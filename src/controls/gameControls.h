#pragma once

#include "gameControlEvents.h"
#include "controls.h"

void resetGameControls();
TeamGeneralInfo *selectTeamForUpdate();
std::function<void()> updateTeamControls(TeamGeneralInfo *team);
GameControlEvents getPlayerEvents(PlayerNumber player);
bool isPlayerFiring(PlayerNumber player);
bool getFireStartedAndBumpFireCounter(bool currentFire, PlayerNumber player = kPlayer1);
int16_t eventsToDirection(GameControlEvents events);
GameControlEvents directionToEvents(int16_t direction);
bool isAnyPlayerFiring();
