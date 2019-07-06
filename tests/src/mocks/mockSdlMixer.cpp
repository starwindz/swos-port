int Mix_HaltChannel(int) { return 1; }
int Mix_VolumeChunk(Mix_Chunk *, int) { return 0; }
int Mix_PlayChannelTimed(int, Mix_Chunk *, int, int) { return 0; }
int Mix_Playing(int channel) { return 0; }
Mix_Chunk *Mix_LoadWAV_RW(SDL_RWops *, int) { return nullptr; }
void Mix_FreeChunk(Mix_Chunk *) {}
