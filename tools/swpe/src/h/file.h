#pragma once

enum FileFlags {
    FF_NONE = 0,
    FF_READ = 1,
    FF_WRITE = 2,
    FF_SEQ_SCAN = 4,
    FF_SHARE_READ = 8,
    FF_SHARE_WRITE = 16,
    FF_OPEN_ALWAYS = 32
};

#define ERR_HANDLE INVALID_HANDLE_VALUE

#ifdef DEBUG
HANDLE FOpenDbg(const char *fname, const uint flags, const char *file, const int line);
HANDLE FCreateDbg(const char *fname, const uint flags, const char *file, const int line);
void FCloseDbg(HANDLE h, const char *file, const int line);
#define FClose(a) FCloseDbg(a, __FILE__, __LINE__)
#define FOpen(a, b) FOpenDbg(a, b, __FILE__, __LINE__)
#define FCreate(a, b) FCreateDbg(a, b, __FILE__, __LINE__)
void SetTrackFiles(bool state);
#else
HANDLE FOpen(const char *fname, const uint flags);
HANDLE FCreate(const char *fname, const uint flags);
#define FClose(a) CloseHandle(a)
#endif

int getNumOpenFiles();
int getMaxOpenFiles();
void logOpenFiles();
