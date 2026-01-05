#pragma once

constexpr int kNumFormationEntries = 18;

void initBenchBeforeMatch();
void updateBench();
bool inBench();
bool inBenchMenus();
int getBenchY();
int getOpponentBenchY();
void swapBenchWithOpponent();
void setBenchOff();
FixedPoint benchCameraX();
const PlayerInfo& getBenchPlayer(int index);
int getBenchPlayerPosition(int index);
