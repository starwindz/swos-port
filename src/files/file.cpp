#include "file.h"
#include "file.h"
#include "log.h"
#include "util.h"
#include "selectFilesMenu.h"
#include <dirent.h>

static std::string m_rootDir;

// Open a file with consideration to SWOS root directory
FILE *openFile(const char *path, const char *mode /* = "rb" */)
{
    return fopen((m_rootDir + path).c_str(), mode);
}

// Return size of the given file
int getFileSize(const char *path, bool required /* = true */)
{
    struct stat st;

    if (stat((m_rootDir + path).c_str(), &st) != 0) {
        if (required)
            errorExit("Failed to stat file %s", path);

        return -1;
    }

    return st.st_size;
}

// Internal routine that does all the work.
static int loadFile(const char *path, void *buffer, int bufferSize, bool required)
{
    auto f = openFile(path);
    if (!f) {
        if (required)
            errorExit("Could not open %s for reading", path);

        return -1;
    }

    setbuf(f, nullptr);
    bool plural = bufferSize != 1;
    logInfo("Loading `%s' [%s byte%s]", path, formatNumberWithCommas(bufferSize).c_str(), plural ? "s" : "");

    bool readOk = fread(buffer, bufferSize, 1, f) == 1;
    fclose(f);

    if (!readOk) {
        if (required)
            errorExit("Error reading file %s", path);

        return -1;
    }

    return bufferSize;
}

static bool isAllowedExtensionStartChar(char c)
{
    return c == 'D' || c == 'P' || c == 'S' || c == 'T' || c == 'H' || c == 'C';
}

static FileList findFiles(const char *extension)
{
    assert(extension && extension[0] == '.');

    FileList result;

    logInfo("Searching for files, extension: .%s", extension);

    auto dir = opendir(!m_rootDir.empty() ? m_rootDir.c_str() : ".");
    if (!dir)
        sdlErrorExit("Couldn't open SWOS root directory");

    bool acceptAll = extension[0] == '*';

    for (dirent *entry; entry = readdir(dir); ) {
        if (!entry->d_namlen)
            continue;

        auto dot = entry->d_name + entry->d_namlen - 1;
        while (dot >= entry->d_name && *dot != '.')
            dot--;

        if (dot < entry->d_name)
            continue;

        if (!acceptAll) {
            if (entry->d_namlen < 5)
                continue;

            bool match = true;

            for (int i = 0; i < 3; i++) {
                if (!dot[i + 1])
                    break;
                if (toupper(extension[i + 1]) != toupper(dot[i + 1])) {
                    match = false;
                    break;
                }
            }

            if (!match)
                continue;
        } else {
            // skip "." and ".." entries
            if (entry->d_namlen == 1 && entry->d_name[0] == '.' ||
                entry->d_namlen == 2 && entry->d_name[0] == '.' && entry->d_name[1] == '.')
                continue;
        }

        char c = toupper(dot[1]);
        if (!isAllowedExtensionStartChar(c))
            continue;

        // must make it upper case or SWOS will reject it like a cruel mother
        toUpper(entry->d_name);

        result.emplace_back(entry->d_name, dot - entry->d_name - 1);
    }

    closedir(dir);

    logInfo("Found %d files", result.size());

    return result;
}

int loadFile(const char *path, void *buffer, bool required /* = true */)
{
    auto size = getFileSize(path, required);
    if (size < 0)
        return -1;

    return loadFile(path, buffer, size, required);
}

// loadFile
//
// in:
//      path         - path of the file to load
//      bufferOffset - number of additional bytes at the start of the buffer that will remain uninitialized
// out:
//      pair of allocated buffer (which caller must free) and its size; nullptr/0 in case of error
//
// Loads file with given path into memory. Returns file contents in a buffer allocated from the heap together with
// its size. Can optionally extend the buffer by a given size, leaving that much initial bytes uninitialized,
// convenient for writing custom data at the start of the buffer (such as header).
//
std::pair<char *, size_t> loadFile(const char *path, size_t bufferOffset /* = 0 */)
{
    auto size = getFileSize(path, false);
    if (size < 0)
        return {};

    auto buffer = new char[size + bufferOffset];
    auto dest = buffer + bufferOffset;

    if (loadFile(path, dest, size, false) != size)
        delete[] buffer;

    return { buffer, size + bufferOffset };
}

static bool getFilenameAndExtension()
{
    auto savedExtension = D0.asDword();

    char extension[5];
    *reinterpret_cast<dword *>(extension) = D0;
    std::swap(extension[0], extension[3]);
    std::swap(extension[1], extension[2]);
    extension[4] = '\0';

    auto menuTitle = A0.asPtr();
    auto files = findFiles(extension);

    auto selectedFilename = showSelectFilesMenu(menuTitle, false, files);

    static char filenameBuffer[kMaxPath];
    strncpy_s(filenameBuffer, sizeof(filenameBuffer), selectedFilename.c_str(), _TRUNCATE);

    A0 = filenameBuffer;
    D1 = savedExtension;

    return filenameBuffer[0] != '\0';
}

// GetFilenameAndExtension
//
// Creates a menu from files with extension in D1, and returns the name of selected file in A0.
//
// in:
//      D0 - extension to search for, 32-bit char constant, dot is in highest byte
//      A0 -> menu title
//      A1 ->   -||-
// out:
//      D0 - 1 = success (zero flag set)
//           0 = no files selected (zero flag clear)
//      D1 - extension
//      A0 -> selected filename
//      zero flag - clear = error
//                  set = OK
// globals:
//   read:
//     searchAlsoTacAndHil - if -1, .TAC and .HIL files
//                           are not searched for
//   write:
//     selectedFilename
//     extension
//
__declspec(naked) int SWOS::GetFilenameAndExtension()
{
    __asm {
        call getFilenameAndExtension
        call setZeroFlagAndD0
    }
}

static bool selectFileToSaveDialog()
{
    return false;
}

// SelectFileToSaveDialog
//
// in:
//      D0 - file extension
//      A0 - buffer for selected filename
//      A1 - menu header
// out:
//      A0 -> selected file name
//      zero flag - set = canceled or error, clear = OK
//      (D0 also)
//
__declspec(naked) int SWOS::SelectFileToSaveDialog()
{
    __asm {
        call selectFileToSaveDialog
        call setZeroFlagAndD0
    }
}

// LoadFile
//
// in:
//      A0 -> file name
//      A1 -> buffer
// out:
//      zero flag set - all OK
//        -||-  clear - error   [actually will never return this, so always return with zero flag set]
//      D0 contains result (0 - OK)
//      D1 returns file size
//
// Load file contents into given buffer. If the file can't be found, or some error happens,
// writes the name of the file to the console, and terminates the program.
//
void SWOS::LoadFile()
{
    auto filename = A0.as<const char *>();
    auto buffer = A1.as<char *>();

    auto savedSelTeamsPtr = selTeamsPtr; // why does a load file routine do this?!?!

    D0 = 0;
    D1 = loadFile(filename, buffer);

    selTeamsPtr = savedSelTeamsPtr;

    _asm xor eax, eax
}

// WriteFile
//
// in:
//      A0 -> filename
//      A1 -> buffer
//      D1 = buffer size
//
// out:
//      D0 = 0 - success
//           1 - failure
//      zero flag set - all OK
//        -||-  clear - error
//
void SWOS::WriteFile()
{
    auto filename = A0.as<const char *>();
    auto buffer = A1.as<char *>();
    auto bufferSize = D1;

    logInfo("Writing file `%s' [%s bytes]", filename, formatNumberWithCommas(bufferSize).c_str());

    auto f = openFile(filename, "wb");
    bool ok = f && fwrite(buffer, 1, bufferSize, f) == bufferSize;

    if (ok) {
        fclose(f);
        __asm {
            xor eax, eax
            mov D0, eax
        }
    } else {
        __asm {
            xor eax, eax
            inc eax
            mov D0, eax
        }
    }
}

void setRootDir(const char *dir)
{
    m_rootDir = dir;
    if (m_rootDir.back() != '\\')
        m_rootDir += '\\';
}

std::string rootDir()
{
    return m_rootDir;
}

std::string pathInRootDir(const char *filename)
{
    return m_rootDir.empty() ? filename : m_rootDir + filename;
}


char getDirSeparator()
{
    return '\\';
}

bool isFileSystemCaseInsensitive()
{
    return true;
}

std::string joinPaths(const char *path1, const char *path2)
{
    return std::string(path1) + getDirSeparator() + path2;
}

bool dirExists(const char *path)
{
#ifdef _WIN32
    auto fileAttributes = ::GetFileAttributes(path);
    return fileAttributes != INVALID_FILE_ATTRIBUTES && (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
# error Please implement platform specific dirExists()
#endif
}

// Returns pointer to last dot in the string, if found, or to the last string character otherwise.
const char *getFileExtension(const char *path)
{
    auto result = strrchr(path, '.');
    return result ? result : path;
}

const char *getBaseName(const char *path)
{
    auto result = strrchr(path, getDirSeparator());
    return result ? result + 1 : path;
}
