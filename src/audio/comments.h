#pragma once

void loadCommentary();
void loadAndPlayEndGameComment();
void clearCommentsSampleCache();
void initCommentsBeforeTheGame();
void enqueueTacticsChangedSample();
void enqueueSubstituteSample();
void playEnqueuedSamples();
bool commenteryOnChannelFinished(int channel);
void toggleMuteCommentary();
