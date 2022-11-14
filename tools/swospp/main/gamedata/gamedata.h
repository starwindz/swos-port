#pragma once

extern "C" {
    void initGameData();
    void stopRecordingGame();
}

void setReplayFile(const char *filename, int length, bool fastForward);
