#include "selectFilesMenu.h"
#include "selectFiles.mnu.h"

static const char *m_menuTitle;
static bool m_saving;
static FileList m_filenames;
static std::string m_selectedFilename;
static std::string m_saveFilename;

std::string showSelectFilesMenu(const char *menuTitle, bool saving, const FileList& filenames)
{
    assert(menuTitle);

    m_menuTitle = menuTitle;
    m_saving = saving;
    m_filenames = filenames;

    showMenu(selectFilesMenu);

    return m_selectedFilename;
}

static void selectInitialEntry()
{
    using namespace SelectFilesMenu;

    if (m_saving)
        setCurrentEntry(m_saveFilename.empty() ? inputSaveFilename : saveLabel);
    else if (m_filenames.empty())
        setCurrentEntry(SelectFilesMenu::abort);
}

static void sortFilenames()
{
    // sort by extension first, filename second
    std::sort(m_filenames.begin(), m_filenames.end(), [](const auto& file1, const auto& file2) {
        auto name1 = file1.first.c_str();
        auto name2 = file2.first.c_str();

        auto ext1 = name1 + file1.second;
        auto ext2 = name2 + file2.second;

        auto result = _stricmp(ext1, ext2);
        if (!result)
            result = _stricmp(name1, name2);

        return result < 0;
    });
}

static void setSaveFilename()
{
    using namespace SelectFilesMenu;

    if (m_saving) {
        auto saveFilenameEntry = getMenuEntryAddress(inputSaveFilename);

        auto dest = saveFilenameEntry->u2.string;
        for (size_t i = 0; i < m_saveFilename.size() && m_saveFilename[i] != '.'; i++)
            *dest++ = m_saveFilename[i];

        saveFilenameEntry->invisible = false;

        if (!m_saveFilename.empty())
            getMenuEntryAddress(saveLabel)->invisible = false;
    }
}

static std::pair<int, bool> splitIntoColumns(int (&numEntriesPerColumn)[3])
{
    int numColumns = 3;
    bool longNames = false;

    if (m_filenames.size() <= 8) {
        numEntriesPerColumn[0] = 0;
        numEntriesPerColumn[1] = 0;
        numEntriesPerColumn[2] = m_filenames.size();
    } else {
        longNames = std::any_of(m_filenames.begin(), m_filenames.end(), [](const auto& file) { return file.first.size() > 11; });
        if (longNames) {
            numColumns = 2;
            numEntriesPerColumn[2] = 0;
        }

        auto quotRem = std::div(m_filenames.size(), numColumns);
        for (int i = 0; i < numColumns; i++) {
            numEntriesPerColumn[i] = quotRem.quot;
            numEntriesPerColumn[i] += quotRem.rem > i;
        }
    }

    return { numColumns, longNames };
}

static void repositionFilenameEntries(int(&numEntriesPerColumn)[3], int numColumns, bool longNames)
{
    using namespace SelectFilesMenu;

    //RepositionSelectedFileEntries <- account for width change
    //fix width of entries based on number of columns
    int maxNumColumns = std::max({ numEntriesPerColumn[0], numEntriesPerColumn[1], numEntriesPerColumn[2] });

    auto titleEntry = getMenuEntryAddress(title);

    int startY = titleEntry->y;
    if (!titleEntry->invisible)
        startY += titleEntry->height;

    auto lastEntry = getMenuEntryAddress(m_saving ? inputSaveFilename : SelectFilesMenu::abort);
    int verticalSlack = (lastEntry->y - startY) / 2;

    startY += verticalSlack;
    auto filenameEntry = getMenuEntryAddress(fileEntry_00);

    for (int i = 0; i < kMaxColumns; i++)
        for (int j = 0; j < kMaxEntriesPerColumn; j++)
            filenameEntry->y = startY + j * (kFilenameHeight + 1);
}

static void selectFilesOnInit()
{
    using namespace SelectFilesMenu;

    getMenuEntryAddress(title)->u2.string = const_cast<char *>(m_menuTitle);

    selectInitialEntry();
    sortFilenames();
    setSaveFilename();

    int numColumns;
    int numEntriesPerColumn[3];
    bool longNames;

    std::tie(numColumns, longNames) = splitIntoColumns(numEntriesPerColumn);

    repositionFilenameEntries(numEntriesPerColumn, numColumns, longNames);

    //assign filenames to menu entities
    //handle scroll arrows
}

static void selectFilesOnReturn()
{
    ;
}

static void drawDescription()
{
    ;
}

static void selectFile()
{
    ;
}

static void abortSaveFile()
{
    ;
}

static void inputFilenameToSave()
{
    ;
}

static void saveFileSelected()
{
    ;
}
