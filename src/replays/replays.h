#pragma once

void showReplaysMenu();

void initReplays();

void initNewReplay();
void finishCurrentReplay();

void startReplay();
bool gotReplay();
bool highlightsValid();
bool loadReplayFile(const char *path);
bool saveReplayFile(const char *path, bool overwrite = true);
bool loadHighlightsFile(const char *path);

void showHighlights();
void toggleFastReplay();

void saveCoordinatesForHighlights(int spriteIndex, int x, int y);
