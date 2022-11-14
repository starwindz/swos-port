#pragma once

/* had to use weird names because of namespace pollution */
#ifdef DEBUG
void *omallocDbg(uint size, const char *file, const int line);
void *xmallocDbg(uint size, const char *file, const int line);
void xfreeDbg(void *p, char *file, int line);
#define omalloc(a) omallocDbg(a, __FILE__, __LINE__)
#define xmalloc(a) xmallocDbg(a, __FILE__, __LINE__)
#define xfree(a) xfreeDbg(a, __FILE__, __LINE__)
void SetTrackAlloc(bool state);
void ShowMemoryAndFilesUsage();
#else
void *omalloc(uint size);
void *xmalloc(uint size);
#define xfree(a) GlobalFree(a)
#endif
