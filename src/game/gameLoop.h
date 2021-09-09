#pragma once

void gameLoop(TeamGame *topTeam, TeamGame *bottomTeam);
void showStadiumScreenAndFadeOutMusic(TeamGame *topTeam, TeamGame *bottomTeam, int maxSubstitutes);
void requestFadeAndSaveReplay();
void requestFadeAndInstantReplay();
void requestFadeAndReplayHighlights();
bool isMatchRunning();
