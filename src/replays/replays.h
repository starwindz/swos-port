#pragma once

static constexpr char kReplaysDir[] = { "replays" };
static constexpr char kHighlightsDir[] = { "highlights" };

#include "ReplayDataStorage.h"

using FileStatus = ReplayDataStorage::FileStatus;

struct GameStats;

void initReplays();
void initNewReplay();
void refreshReplayGameData();
void finishCurrentReplay();

void startNewHighlightsFrame();
void saveCoordinatesForHighlights(int spriteIndex, FixedPoint x, FixedPoint y);
void showStatsForHighlights(const GameStats& stats);

bool replayingNow();
bool gotReplay();
bool gotHighlights();

void saveHighlightScene();

void playInstantReplay(bool userRequested);
void playHighlights(bool inGame);
void playFullReplay();

FileStatus loadHighlightsFile(const char *path);
FileStatus loadReplayFile(const char *path);
bool saveHighlightsFile(const char *path, bool overwrite = true);
bool saveReplayFile(const char *path, bool overwrite = true);
