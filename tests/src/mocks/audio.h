#pragma once

int getMusicVolume();
std::pair<bool, bool> isKnownExtension(const char *filename, size_t len);

void saveAudioOptions(CSimpleIni& ini);
void loadAudioOptions(const CSimpleIniA& ini);

void initAudio();
void finishAudio();
void initGameAudio();
void resetGameAudio();
void ensureMenuAudioFrequency();
int playIntroSample(void *buffer, int size, int volume, int loopCount);

void showAudioOptionsMenu();
