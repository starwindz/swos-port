#pragma once

// provide a replacement for dirent.h, with fake functions defined in mockFile.cpp

#define PATH_MAX MAX_PATH

struct dirent {
    long d_ino;
    long d_off;
    unsigned short d_reclen;
    size_t d_namlen;
    int d_type;
    char d_name[PATH_MAX + 1];
};

struct DIR {
    dirent ent;
    size_t currentNodeIndex = 0;
};

DIR *opendir(const char *dirname);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
