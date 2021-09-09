#pragma once

void initMatch(TeamGame *topTeam, TeamGame *bottomTeam, bool saveOrRestoreTeams);
void matchEnded();
void startMainGameLoop();
void checkGlobalKeyboardShortcuts(SDL_Scancode scanCode, bool pressed);
void checkGameKeys();
void updateCursor(bool matchRunning);
bool isMatchRunning();
bool isGamePaused();
void pauseTheGame();
void togglePause();
