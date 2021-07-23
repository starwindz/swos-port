#pragma once

void loadCommentary();
void clearCommentsSampleCache();
void initCommentsBeforeTheGame();
void enqueueTacticsChangedSample();
void enqueueSubstituteSample();
void playEnqueuedSamples();
bool commenteryOnChannelFinished(int channel);
SwosDataPointer<const char> *getOnDemandSampleTable();
