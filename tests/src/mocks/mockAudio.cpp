#include "audio.h"

int getMusicVolume()
{
    return 0;
}

void saveAudioOptions(CSimpleIni& ini) {}
void loadAudioOptions(const CSimpleIniA& ini) {}

void initAudio() {}
void finishAudio() {}
void initGameAudio() {}
void resetGameAudio() {}
void ensureMenuAudioFrequency() {}

int playIntroSample(void *buffer, int size, int volume, int loopCount)
{
    return 0;
}

void showAudioOptionsMenu() {}

void SWOS::StopAudio() {}
void SWOS::PlaySoundSample() {}
