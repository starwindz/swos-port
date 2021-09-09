#include "audio.h"

int getMusicVolume()
{
    return 0;
}

void saveAudioOptions(CSimpleIni& ini) {}
void loadAudioOptions(const CSimpleIniA& ini) {}

void initAudio() {}
void stopAudio() {}
void finishAudio() {}
void initGameAudio() {}
void resetGameAudio() {}
void ensureMenuAudioFrequency() {}

bool soundEnabled() { return true; }
void setSoundEnabled(bool enabled) {}
bool musicEnabled() { return true; }
void setMusicEnabled(bool enabled) {}
bool commentaryEnabled() { return true; }
void setCommentaryEnabled(bool enabled) {}

int playIntroSample(void *buffer, int size, int volume, int loopCount)
{
    return 0;
}

void showAudioOptionsMenu() {}
