#pragma once

void resetMockSdlMixer();
int numTimesPlayChunkCalled();
const std::vector<Mix_Chunk *>& getLastPlayedChunks();
Mix_Chunk *getLastPlayedChunk();
int numberOfChannelsPlaying();
