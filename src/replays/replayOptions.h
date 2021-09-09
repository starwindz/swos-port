#pragma once

void loadReplayOptions(const CSimpleIni& ini);
void saveReplayOptions(CSimpleIni& ini);

bool getAutoSaveReplays();
void setAutoSaveReplays(bool autoSave);
bool getShowReplayPercentage();
void toggleShowReplayPercentage();
