#pragma once

#include "SoundSample.h"

enum SfxSampleIndex {
    kBackgroundCrowd, kBounce, kHomeGoal, kKick, kWhistle, kMissGoal, kEndGameWhistle, kFoulWhistle, kChant4l, kChant10l, kChant8l,
    kNumSoundEffects,
};

using SfxSamplesArray = std::array<SoundSample, kNumSoundEffects>;

void initSfxBeforeTheGame();
SfxSamplesArray& sfxSamples();
void playCrowdNoiseSample();
void stopBackgroudCrowdNoise();
