#pragma once

struct MockFile {
    MockFile(const char *path, const char *data, int size) : path(path), data(data), size(size) {}
    MockFile(const char *path, unsigned const char *data, int size)
        : path(path), data(reinterpret_cast<const char *>(data)), size(size) {}
    MockFile(const char *path) : path(path) {}
    const char *path;
    const char *data = nullptr;
    int size = 0;
};

using MockFileList = std::vector<MockFile>;

void enableFileMocking(bool mockingEnabled);
void resetFakeFiles();
void addFakeFiles(const MockFileList& files);
void addFakeFile(const MockFile& file);
void addFakeDirectory(const char *path);
bool deleteFakeFile(const char *path);
int getNumFakeFiles();
bool setFileAsCorrupted(const char *path, bool corrupted = true);
bool fakeFilesEqualByContent(const char *path1, const char *path2);
const char *getFakeFileData(const char *path, int& size, int& numWrites);
int getFakeFileSize(const char *path);
