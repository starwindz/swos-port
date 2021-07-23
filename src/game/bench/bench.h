#pragma once

constexpr int kNumFormationEntries = 18;

void initBenchBeforeMatch();
void updateBench();
bool inBench();
bool isCameraLeavingBench();
void clearCameraLeavingBench();
void setCameraLeavingBench();
int getBenchY();
int getOpponentBenchY();
void swapBenchWithOpponent();
bool getBenchOff();
void setBenchOff();
int benchCameraX();
const PlayerGame& getBenchPlayer(int index);
int getBenchPlayerPosition(int index);
