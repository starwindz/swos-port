#include "file.h"
#include "log.h"
#include "util.h"
#include "render.h"
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

int loadFile(const char *path, void *buffer, bool required /* = true */)
{
    auto size = getFileSize(path, required);
    if (size < 0)
        return -1;

    return loadFile(path, buffer, size, required);
}

// Load entire file into allocated buffer that caller needs to free
std::pair<char *, size_t> loadFile(const char *path, size_t offset /* = 0 */)
{
    auto size = getFileSize(path, false);
    if (size < 0)
        return {};

    auto buffer = new char[size + offset];

    if (loadFile(path, buffer + offset, size, false) != size)
        delete[] buffer;

    return { buffer, size + offset };
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
    fclose(f);

    if (ok) {
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

// in:
//      A1 -> var to write number of files found (dword), followed
//            by a buffer for filenames (512 x 32 bytes)
// globals:
//      g_extension - dword that holds filter extension, char by char
//
// Fills buffer in A1:
//   word - number of files
//   file names (0 terminated)
//   The caller will assume each file name starts at 32 byte block
//
void SWOS::FindFiles()
{
    constexpr int kMaxFindFiles = 45;
    constexpr int kMaxFilenameLen = 31;

    std::string extension(g_extension, 3);
    if (strlen(g_extension))
        std::reverse(extension.begin(), extension.end());
    else
        extension = "*";

    logInfo("Searching for files, extension .%s", extension.c_str());

    auto dir = opendir(!m_rootDir.empty() ? m_rootDir.c_str() : ".");
    if (!dir)
        sdlErrorExit("Couldn't open SWOS root directory");

    int numFiles = 0;
    auto filenamesBuffer = (char (*)[kMaxFilenameLen + 1])(A1.asPtr() + 4);

    bool acceptAll = !reinterpret_cast<dword *>(g_extension)[0] || g_extension[0] == '*' && g_extension[1] == '.';

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

            // this will not work for 1 and 2-char extensions
            if (g_extension[2] != toupper(dot[1]) || g_extension[1] != toupper(dot[2]) || g_extension[0] != toupper(dot[3]))
                continue;
        } else {
            // skip "." and ".." entries
            if (entry->d_namlen == 1 && entry->d_name[0] == '.' ||
                entry->d_namlen == 2 && entry->d_name[0] == '.' && entry->d_name[1] == '.')
                continue;
        }

        char c = toupper(dot[1]);
        if (c != 'D' && c != 'P' && c != 'S' && c != 'T' && c != 'H' && c != 'C')
            continue;

        if (entry->d_namlen > kMaxFilenameLen) {
            logWarn("Too long filename (>%d chars): %s", kMaxFilenameLen, entry->d_name);
            continue;
        }

        if (!acceptAll && numFiles >= kMaxFindFiles) {
            logWarn("Too many files in SWOS directory (more than %d)", kMaxFindFiles);
            break;
        }

        numFiles++;
        assert(entry->d_namlen < NAME_MAX);
        memcpy(filenamesBuffer, entry->d_name, entry->d_namlen + 1);

        // must make it upper case or SWOS will reject it like a cruel mother
        for (char *p = *filenamesBuffer; *p; p++)
            *p = toupper(*p);

        filenamesBuffer++;
    }

    closedir(dir);

    auto numFilesPtr = A1.as<int *>();
    *numFilesPtr = numFiles;

    logInfo("Found %d files", numFiles);

    // gotta return with zero flag set
    _asm xor eax, eax
}

void SWOS::FindFilesExtensionPresent()
{
    FindFiles();
}

// in:
//     D0 - pitch file number(0 - based)
//
void SWOS::ReadPitchFile()
{
    assert(D0 < 6);

    logInfo("Loading pitch file %d", D0.data);

    // this runs right after virtual screen for menus is trashed; restore it so it can fade-out properly
    // also note that screen pitch is changed by this point
    memcpy(linAdr384k, linAdr384k + 2 * kVgaScreenSize, kVgaScreenSize);
    for (int i = 0; i < kVgaHeight; i++)
        memcpy(linAdr384k + i * screenWidth, linAdr384k + 2 * kVgaScreenSize + i * kVgaWidth, kVgaWidth);

    memset(g_pitchDatBuffer, 0, sizeof(g_pitchDatBuffer));
    memset(g_currentMenu, 0, skillsPerQuadrant - g_currentMenu);

    auto pitchFilenamesTable = &ofsPitch1Dat;
    auto pitchFilename = pitchFilenamesTable[D0];
    auto buffer = g_pitchDatBuffer + 180;

    if (auto file = openFile(pitchFilename)) {
        for (int i = 0; i < 55; i++) {
            if (!fread(buffer, 168, 1, file))
                logWarn("Failed reading pitch file %s, line %d", pitchFilename, i);
            buffer += 176;
        }
        fclose(file);
    }

    auto blkFilename = aPitch1_blk + 16 * D0;
    loadFile(blkFilename, g_pitchPatterns);
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
