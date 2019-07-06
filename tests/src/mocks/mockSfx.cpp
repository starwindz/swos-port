#include "sfx.h"

void initSfxBeforeTheGame() {}
static SfxSamplesArray m_samplesArray;
SfxSamplesArray& sfxSamples() { return m_samplesArray; }
void playCrowdNoiseSample() {}
void stopBackgroudCrowdNoise() {}

void SWOS::LoadSoundEffects() {}
void SWOS::PlayCrowdNoiseSample() {}
void SWOS::PlayMissGoalSample() {}
void SWOS::PlayHomeGoalSample() {}
void SWOS::PlayAwayGoalSample() {}
void SWOS::PlayRefereeWhistleSample() {}
void SWOS::PlayEndGameWhistleSample() {}
void SWOS::PlayFoulWhistleSample() {}
void SWOS::PlayKickSample() {}
void SWOS::PlayBallBounceSample() {}
