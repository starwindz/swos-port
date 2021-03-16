#include "menus.h"
#include "menuMouse.h"
#include "log.h"
#include "util.h"
#include "render.h"
#include "sprites.h"
#include "options.h"
#include "controls.h"
#include "menuControls.h"
#include "mainMenu.h"

#ifdef SWOS_TEST
# include "unitTest.h"
#endif

void MenuEntry::copyString(const char *str)
{
    assert(type == kEntryString);
    strncpy_s(string(), str, kStdMenuTextSize);
}

void setEntriesVisibility(const std::vector<int>& entryIndices, bool visible)
{
    auto currentMenu = getCurrentMenu();

    for (auto entryIndex : entryIndices) {
        assert(entryIndex >= 0 && entryIndex < currentMenu->numEntries);
        currentMenu->entries()[entryIndex].setVisible(visible);
    }
}

void selectEntry(MenuEntry *entry, int controlMask /* = kShortFireMask */)
{
    assert(entry && entry->onSelect);
    if (entry && entry->onSelect) {
        save68kRegisters();

        swos.controlMask = controlMask;

        A5 = entry;
        auto func = entry->onSelect;
        SAFE_INVOKE(func);

        restore68kRegisters();
    }
}

void selectEntry(int ordinal)
{
    assert(ordinal >= 0 && ordinal < 255 && ordinal < getCurrentMenu()->numEntries);

    if (ordinal >= 0 && ordinal < 255) {
        auto entry = getMenuEntry(ordinal);
        selectEntry(entry);
    }
}

void SWOS::FlipInMenu()
{
    frameDelay(2);
    updateScreen();
}

static void menuDelay()
{
#ifdef DEBUG
# ifdef _WIN32
    assert(_CrtCheckMemory());
# endif
    extern void checkMemory();
    checkMemory();
#endif

    if (!swos.menuCycleTimer) {
        timerProc();
        timerProc();    // imitate int 8 ;)
    }
}

static void menuProcCycle()
{
    menuDelay();
    SWOS::MenuProc();
}

void SWOS::WaitRetrace()
{
    menuDelay();
}

void SWOS::MainMenu()
{
    logInfo("Initializing main menu");
    InitMainMenu();

    while (true)
        menuProcCycle();
}

static const char *entryText(int entryOrdinal)
{
    auto entry = getMenuEntry(entryOrdinal);

    static char buf[16];

    switch (entry->type) {
    case kEntryString: return entry->string();
    case kEntryMultilineText: return entry->fg.multiLineText.asCharPtr() + 1;
    case kEntrySprite2:
    case kEntryNumber: return SDL_itoa(entry->fg.number, buf, 10);
    default: return "/";
    }
}

// These are macros instead of functions because we need to save these variables for each level of nesting;
// being functions they can't create local variables on callers stack, and then we'd need explicit stacks.
#define saveCurrentMenu() \
    auto currentMenu = getCurrentMenu(); \
    auto savedEntry = currentMenu->selectedEntry ? currentMenu->selectedEntry->ordinal : 0; \
    auto savedMenu = getCurrentPackedMenu(); \

#define restorePreviousMenu() \
    restoreMenu(savedMenu, savedEntry); \

void showMenu(const BaseMenu& menu)
{
    swos.g_scanCode = 0;

    resetControls();

    menuMouseOnAboutToShowNewMenu();

    saveCurrentMenu();
    logInfo("Showing menu %#x [%s], previous menu is %#x", &menu, entryText(savedEntry), savedMenu);

    auto memoryMark = SwosVM::markAllMemory();

    swos.g_exitMenu = 0;
    activateMenu(&menu);

    while (!swos.g_exitMenu) {
        menuProcCycle();
#ifdef SWOS_TEST
        // ask unit tests how long they want to run menu proc
        if (SWOS_UnitTest::exitMenuProc())
            return;
#endif
    }

    swos.g_exitMenu = 0;
    SwosVM::releaseAllMemory(memoryMark);

    restorePreviousMenu();

    menuMouseOnOldMenuRestored();

    logInfo("Menu %#x finished, restoring menu %#x", &menu, savedMenu);
}

// ShowMenu
//
// in:
//     A6 -> menu to show on screen (packed form)
//
// Shows this menu and blocks, returns only when it's exited.
// And then returns to the previous menu and it becomes the active menu.
//
void SWOS::ShowMenu()
{
    auto menu = A6.as<BaseMenu *>();
    showMenu(*menu);
}

// called by SWOS, when we're starting the game
void SWOS::ToMainGameLoop()
{
    logInfo("Starting the game...");

    saveCurrentMenu();
    safeInvokeWithSaved68kRegisters(StartMainGameLoop);
    restorePreviousMenu();
}

__declspec(naked) void SWOS::DoUnchainSpriteInMenus_OnEnter()
{
#ifdef SWOS_VM
    SwosVM::ecx = 0;
#else
    // only cx is loaded, but later whole ecx is tested; make this right
    _asm {
        xor ecx, ecx
        retn
    }
#endif
}

// ActivateMenu
//
// in:
//     A6 -> menu to activate
// globals:
//     g_currentMenu - destination for unpacked menu.
//
// Unpacks given static menu into g_currentMenu. Executes menu initialization function, if present.
// Sometimes called directly by some menu functions, causing it to skip the menu stack system, and the
// newly shown menu will return to the callers caller.
//
void SWOS::ActivateMenu()
{
    activateMenu(A6.asPtr());
}

// SetCurrentEntry
//
//  in:
//      D0 - number of menu entry
//
//  Sets current entry in the current menu.
//
void SWOS::SetCurrentEntry()
{
    auto currentMenu = getCurrentMenu();
    assert(D0.asWord() < currentMenu->numEntries);

    auto newSelectedEntry = getMenuEntry(D0.asWord());
    currentMenu->selectedEntry = newSelectedEntry;;
}

void activateMenu(const void *menu)
{
    unpackMenu(menu);

    auto currentMenu = getCurrentMenu();
    auto currentEntry = currentMenu->selectedEntry.getRaw();

    if (currentEntry <= 0xffff)
        currentMenu->selectedEntry = getMenuEntry(currentEntry);

    if (currentMenu->onInit) {
        SDL_UNUSED auto menuMark = menuAlloc(0);

        A6 = swos.g_currentMenu;
        auto proc = currentMenu->onInit;
        SAFE_INVOKE(proc);

        assert(menuMark == menuAlloc(0));
    }

    assert(currentMenu->numEntries < 256);

    resetMenuMouseData();
}

void SWOS::InitMainMenu()
{
    InitMenuMusic();
    InitMainMenuStuff();
    // speed up starting up in debug mode
#ifdef DEBUG
    swos.menuFade = 0;
    setPalette(swos.g_workingPalette);
#else
    swos.menuFade = 1;
#endif
}

void SWOS::InitMainMenuStuff()
{
    ZeroOutStars();
    swos.twoPlayers = -1;
    swos.flipOnOff = 1;
    swos.inFriendlyMenu = 0;
    swos.isNationalTeam = 0;

    D0 = kGameTypeNoGame;
    InitCareerVariables();

    swos.menuFade = 0;
    swos.g_exitMenu = 0;
    swos.fireResetFlag = 0;
    swos.dseg_10E848 = 0;
    swos.dseg_10E846 = 0;
    swos.dseg_11F41C = -1;
    swos.coachOrPlayer = 1;
    SetDefaultNameAndSurname();
    swos.plNationality = kEng;       // English is the default
    swos.competitionOver = 0;
    InitHighestScorersTable();
    swos.teamsLoaded = 0;
    swos.poolplyrLoaded = 0;
    swos.importTacticsFilename[0] = 0;
    swos.chooseTacticsTeamPtr = nullptr;
    InitUserTactics();

    activateMainMenu();
}

void SWOS::ExitEuropeanChampionshipMenu()
{
    activateMainMenu();
}

void SWOS::AbortTextInputOnEscapeAndRightMouseClick()
{
    if (swos.lastKey == kKeyEscape || SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) {
        swos.convertedKey = kKeyEscape;
        swos.g_inputingText = 0;
    }
}

bool inputNumber(MenuEntry *entry, int maxDigits, int minNum, int maxNum)
{
    assert(entry->type == kEntryNumber);

    int num = static_cast<int16_t>(entry->fg.number);
    assert(num >= minNum && num <= maxNum);

    swos.g_inputingText = -1;

    auto buf = menuAlloc(16);
    SDL_itoa(num, buf, 10);
    assert(static_cast<int>(strlen(buf)) <= maxDigits);

    auto end = buf + strlen(buf);
    *end++ = -1;    // insert cursor
    *end = '\0';

    entry->type = kEntryString;
    entry->setString(buf);

    auto cursorPtr = end - 1;

    while (true) {
        processControlEvents();
        SWOS::DrawMenu();
        SWOS::WaitRetrace();

        SWOS::GetKey();

        while (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) {
            swos.lastKey = kKeyEscape;
            processControlEvents();
            SDL_Delay(15);
        }

        switch (swos.lastKey) {
        case kKeyEnter:
            memmove(cursorPtr, cursorPtr + 1, end - cursorPtr);
            entry->type = kEntryNumber;
            entry->fg.number = atoi(buf);
            swos.g_inputingText = 0;
            return true;

        case kKeyEscape:
            A5 = entry;
            entry->type = kEntryNumber;
            entry->fg.number = num;
            swos.g_inputingText = 0;   // original SWOS leaves it set, wonder if it's significant...
            return false;

        case kKeyLShift:
        case kKeyArrowLeft:
            if (cursorPtr != buf) {
                std::swap(cursorPtr[-1], cursorPtr[0]);
                cursorPtr--;
            }
            break;

        case kKeyRShift:
        case kKeyArrowRight:
            if (cursorPtr[1]) {
                std::swap(cursorPtr[0], cursorPtr[1]);
                cursorPtr++;
            }
            break;

        case kKeyHome:
        case kKeyArrowDown:
            memmove(buf + 1, buf, cursorPtr - buf);
            *(cursorPtr = buf) = kCursorChar;
            break;

        case kKeyEnd:
        case kKeyArrowUp:
            if (cursorPtr[1]) {
                memmove(cursorPtr, cursorPtr + 1, end - cursorPtr);
                *(cursorPtr = end - 1) = kCursorChar;
            }
            break;

        case kKeyBackspace:
            if (cursorPtr != buf) {
                memmove(cursorPtr, cursorPtr + 1, end - cursorPtr);
                *--cursorPtr = kCursorChar;
                end--;
            }
            break;

        case kKeyDelete:
            if (cursorPtr[1]) {
                memmove(cursorPtr, cursorPtr + 1, end - cursorPtr);
                *cursorPtr = kCursorChar;
                end--;
            }
            break;

        case kKeyMinus: case kKeyKpMinus:
            if (cursorPtr != buf || minNum >= 0)
                break;

            swos.convertedKey = '-';
            // assume fall-through

        case kKey0: case kKey1: case kKey2: case kKey3: case kKey4:
        case kKey5: case kKey6: case kKey7: case kKey8: case kKey9:
            if (end - buf - 1 < maxDigits) {
                *cursorPtr++ = swos.convertedKey;
                auto newValue = atoi(buf);

                if (newValue >= minNum && newValue <= maxNum && (newValue != 0 || end == buf + 1 || swos.lastKey != kKey0)) {
                    memmove(cursorPtr + 1, cursorPtr, end++ - cursorPtr + 1);
                    *cursorPtr = kCursorChar;
                } else {
                    *--cursorPtr = kCursorChar;
                }
            }
            break;
        }
    }
}

bool showContinueAbortPrompt(const char *header, const char *continueText,
    const char *abortText, const std::vector<const char *>& messageLines)
{
    using namespace SwosVM;

    assert(messageLines.size() < 7);
    auto mark = getMemoryMark();

    A0 = allocateString(header);
    A2 = allocateString(continueText);
    A3 = allocateString(abortText);

    constexpr int kBufferSize = 256;
    auto buf = allocateMemory(kBufferSize);

    auto p = buf + 1;
    auto sentinel = buf + kBufferSize - 1;

    for (auto line : messageLines) {
        do {
            if (p >= sentinel)
                break;
            *p++ = *line;
        } while (*line++);

        while (*p == ' ')
            p--;

        assert(p > buf);
    }

    *p = '\0';
    buf[0] = static_cast<char>(messageLines.size());

    A1 = buf;
    assert(p - buf + 1 < kBufferSize);

    DoContinueAbortMenu();

    releaseMemory(mark);
    return D0 == 0;
}
