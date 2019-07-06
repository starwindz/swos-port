#pragma once

constexpr int kMaxFilenameLength = 260;

FILE *openFile(const char *path, const char *mode = "rb");
int getFileSize(const char *path, bool required = true);
int loadFile(const char *filename, void *buffer, bool required = true);
std::pair<char *, size_t> loadFile(const char *filename, size_t offset = 0);
void setRootDir(const char *dir);
std::string rootDir();
std::string pathInRootDir(const char *filename);
char getDirSeparator();
std::string joinPaths(const char *path1, const char *path2);
bool dirExists(const char *path);
const char *getFileExtension(const char *path);
const char *getBaseName(const char *path);
