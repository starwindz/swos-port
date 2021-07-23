#pragma once

void initMatch(TeamGame *topTeam, TeamGame *bottomTeam, bool saveOrRestoreTeams);
void matchEnded();
void checkKeyboardShortcuts(SDL_Scancode scanCode, bool pressed);
void updateCursor(bool matchRunning);
bool isMatchRunning();
