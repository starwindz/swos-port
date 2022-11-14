#include "resData.h"
#include "file.h"
#include "unitTest.h"

ResFilenameList findResFiles(const char *extension)
{
    ResFilenameList result;

    traverseDirectory(TEST_DATA_DIR, extension, [&result](const char *filename, auto, auto) {
        result.emplace_back(joinPaths(TEST_DATA_DIR, filename));
        return true;
    });

    return result;
}

std::pair<std::unique_ptr<const char>, size_t> loadResFile(const char *name)
{
    const auto& path = joinPaths(TEST_DATA_DIR, name);
    auto failedToMessage = [&](const char *what) {
        return "Failed to "s + what + " resource file " + name + " at path " + path;
    };
    auto handle = SDL_RWFromFile(path.c_str(), "rb");
    assertMessage(handle, failedToMessage("open"));
    size_t size = SDL_RWsize(handle);
    assertMessage(size > 0, failedToMessage("get size of"));
    auto buffer = new char[size];
    assertMessage(SDL_RWread(handle, buffer, size, 1) == 1, failedToMessage("read"));
    return { std::unique_ptr<const char>(buffer), size };
}
