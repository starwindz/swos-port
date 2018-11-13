#include "menu.h"
#include "log.h"
#include "util.h"
#include "render.h"
#include "music.h"
#include "controls.h"

using ReachabilityMap = std::array<bool, 256>;
static ReachabilityMap m_reachableEntries;

void SWOS::FlipInMenu()
{
    frameDelay(2);
    updateScreen();
}

// this is actually a debugging function, but used by everyone to quickly exit SWOS :)
void SWOS::SetPrevVideoModeEndProgram()
{
    std::exit(0);
}

static void menuDelay()
{
#ifndef NDEBUG
    extern void checkMemory();

    assert(_CrtCheckMemory());
    checkMemory();
#endif

    if (!menuCycleTimer) {
        timerProc();
        timerProc();    // imitate int 8 ;)
    }
}

static void checkMousePosition()
{
    static int lastX = -1, lastY = -1;
    static bool clickedLastFrame;
    static bool selectedLastFrame = true;
    static Uint32 startingTIcks = SDL_GetTicks();

    int x, y;
    auto buttons = SDL_GetMouseState(&x, &y);

    // make sure to discard initial activity while the window is initializing
    if ((x != lastX || y != lastY) && SDL_GetTicks() - startingTIcks > 100)
        selectedLastFrame = false;

    lastX = x;
    lastY = y;

    if (clickedLastFrame) {
        if (!buttons) {
            clickedLastFrame = false;
            selectedLastFrame = false;
        }

        releaseFire();
        return;
    }

    if (selectedLastFrame && !(buttons & SDL_BUTTON(SDL_BUTTON_LEFT)))
        return;

    int windowWidth, windowHeight;
    std::tie(windowWidth, windowHeight) = getWindowSize();

    x = x * kVgaWidth / windowWidth;
    y = y * kVgaHeight / windowHeight;

    if (hasMouseFocus() && vsPtr == linAdr384k) {
        auto currentMenu = reinterpret_cast<Menu *>(g_currentMenu);
        auto entries = reinterpret_cast<MenuEntry *>(g_currentMenu + sizeof(Menu));

        for (size_t i = 0; i < currentMenu->numEntries; i++) {
            auto& entry = entries[i];
            bool insideEntry = x >= entry.x && x < entry.x + entry.width && y >= entry.y && y < entry.y + entry.height;

            if (insideEntry && !entry.invisible && m_reachableEntries[entry.ordinal]) {
                currentMenu->selectedEntry = &entry;
                selectedLastFrame = true;

                bool isPlayMatch = entry.type2 == ENTRY_STRING && entry.u2.string && !strcmp(entry.u2.string, "PLAY MATCH");

                if (buttons & SDL_BUTTON(SDL_BUTTON_LEFT) && !isPlayMatch) {
                    pressFire();
                    clickedLastFrame = true;
                    player1ClearFlag = 1;
                }
            }
        }
    }
}

static void menuProcCycle()
{
    menuDelay();
    checkMousePosition();
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

static const char *entryText(const MenuEntry *entry)
{
    static char buf[16];
    if (entry) {
        switch (entry->type1) {
        case ENTRY_STRING: return entry->u2.string;
        case ENTRY_MULTI_LINE_TEXT: return reinterpret_cast<char *>(entry->u2.multiLineText) + 1;
        case ENTRY_SPRITE2:
        case ENTRY_NUMBER: return _itoa(entry->u2.number, buf, 10);
        }
    }

    return "/";
}

// Find and mark every reachable item starting from the initial one.
static void determineReachableEntries(const MenuBase *menu)
{
    std::fill(m_reachableEntries.begin(), m_reachableEntries.end(), false);

    static_assert(offsetof(MenuEntry, downEntry) == offsetof(MenuEntry, upEntry) + 1 &&
        offsetof(MenuEntry, upEntry) == offsetof(MenuEntry, rightEntry) + 1 &&
        offsetof(MenuEntry, rightEntry) == offsetof(MenuEntry, leftEntry) + 1 &&
        sizeof(MenuEntry::leftEntry) == 1, "MenuEntry(): assumptions about next entries failed");

    std::vector<std::pair<const MenuEntry *, int>> entryStack;
    entryStack.reserve(256);

    auto currentMenu = reinterpret_cast<Menu *>(g_currentMenu);
    auto currentEntry = currentMenu->selectedEntry;
    auto entries = reinterpret_cast<MenuEntry *>(g_currentMenu + sizeof(Menu));

    entryStack.emplace_back(currentEntry, 0);

    while (!entryStack.empty()) {
        const MenuEntry *entry;
        int direction;

        std::tie(entry, direction) = entryStack.back();

        assert(entry && direction >= 0 && direction <= 4);
        assert(entry->ordinal < 256);

        m_reachableEntries[entry->ordinal] = true;

        auto nextEntries = reinterpret_cast<const uint8_t *>(&entry->leftEntry);

        for (; direction < 4; direction++) {
            auto ord = nextEntries[direction];

            if (ord != 255 && !m_reachableEntries[ord]) {
                entryStack.emplace_back(&entries[ord], 0);
                break;
            }
        }

        if (direction >= 4) {
            entryStack.pop_back();
        } else {
            assert(entryStack.size() >= 2);
            entryStack[entryStack.size() - 2].second = direction + 1;
        }
    }
}

// these are macros instead of functions because we need to save these variables for each level of nesting;
// being functions they can't create local variables on callers stack, and then we'd need explicit stacks
#define saveCurrentMenu() \
    auto currentMenu = reinterpret_cast<Menu *>(g_currentMenu); \
    auto savedEntry = currentMenu->selectedEntry; \
    auto savedMenu = g_currentMenuPtr; \

#define restorePreviousMenu() \
    A6 = savedMenu; \
    A5 = savedEntry; \
    SAFE_INVOKE(RestorePreviousMenu); \

// ShowMenu
//
// in:
//     A6 -> menu to show on screen (packed form)
//
// After showing this menu, returns to previous menu.
//
void SWOS::ShowMenu()
{
    g_scanCode = 0;
    controlWord = 0;
    menuStatus = 0;

    auto newMenu = A6.asPtr();
    saveCurrentMenu();
    logInfo("Showing menu %#x [%s], previous menu is %#x", A6.data, entryText(savedEntry), savedMenu);

    g_exitMenu = 0;
    SAFE_INVOKE(PrepareMenu);

    while (!g_exitMenu)
        menuProcCycle();

    menuStatus = 1;
    g_exitMenu = 0;

    restorePreviousMenu();
    determineReachableEntries(savedMenu);

    logInfo("Menu %#x finished, restoring menu %#x", newMenu, g_currentMenuPtr);
}

// called by SWOS, when we're starting the game
void SWOS::ToMainGameLoop()
{
    logInfo("Starting the game...");

    saveCurrentMenu();
    save68kRegisters();

    SAFE_INVOKE(StartMainGameLoop);

    restore68kRegisters();
    restorePreviousMenu();
}

__declspec(naked) void SWOS::DoUnchainSpriteInMenus_OnEnter()
{
    // only cx is loaded, but later whole ecx is tested; make this right
    _asm {
        xor ecx, ecx
        retn
    }
}

// PrepareMenu
//
// in:
//     A6 -> menu to prepare (or activate)
// globals:
//     g_currentMenu - destination for unpacked menu.
//
// Unpack given static menu into g_currentMenu. Execute menu init function, if present.
//
void SWOS::PrepareMenu()
{
    g_currentMenuPtr = A6;
    auto packedMenu = A6.as<const MenuBase *>();

    A0 = A6;
    A1 = g_currentMenu;
    ConvertMenu();

    auto currentMenu = reinterpret_cast<Menu *>(g_currentMenu);
    auto currentEntry = reinterpret_cast<uint32_t>(currentMenu->selectedEntry);

    if (currentEntry <= 0xffff)
        currentMenu->selectedEntry = getMenuEntryAddress(currentEntry);

    if (currentMenu->onInit) {
        A6 = g_currentMenu;
        auto proc = currentMenu->onInit;
        SAFE_INVOKE(proc);
    }

    assert(currentMenu->numEntries < 256);

    determineReachableEntries(packedMenu);
}

void SWOS::InitMainMenu()
{
    InitMenuMusic();
    SAFE_INVOKE(InitMainMenuStuff);
    menuFade = 1;
}

static MenuEntry *findNextEntry(byte nextEntryIndex, int nextEntryDirection)
{
    auto currentMenu = getCurrentMenu();

    while (nextEntryIndex != 255) {
        auto nextEntry = getMenuEntryAddress(nextEntryIndex);

        if (!nextEntry->disabled && !nextEntry->invisible)
            return nextEntry;

        auto newDirection = (&nextEntry->leftDirection)[nextEntryDirection];
        if (newDirection != 255) {
            nextEntryIndex = (&nextEntry->leftEntryDis)[nextEntryDirection];
            nextEntryDirection = newDirection;
            assert(newDirection >= 0 && newDirection <= 3);
            assert(nextEntryIndex != 255);
        } else {
            nextEntryIndex = (&nextEntry->leftEntry)[nextEntryDirection];
        }
    }

    return nullptr;
}

void SWOS::MenuProc()
{
    g_videoSpeedIndex = 50;     // just in case

    ReadTimerDelta();
    DrawMenu();
    menuCycleTimer = 0;         // must come after DrawMenu(), set to 1 to slow down input

    if (menuFade) {
        FadeIn();
        menuFade = 0;
    }

    CheckControls();
    updateSongState();

    auto currentMenu = reinterpret_cast<Menu *>(g_currentMenu);
    MenuEntry *nextEntry = nullptr;

    if (currentMenu->selectedEntry || finalControlsStatus >= 0 && !previousMenuItem) {
        auto activeEntry = currentMenu->selectedEntry;

        int controlMask = 0;
        controlsStatus = 0;

        if (activeEntry->onSelect) {
            if (shortFire)
                controlMask |= 0x20;
            if (fire)
                controlMask |= 0x01;
            if (left)
                controlMask |= 0x02;
            if (right)
                controlMask |= 0x04;
            if (up)
                controlMask |= 0x08;
            if (down)
                controlMask |= 0x10;

            if (finalControlsStatus >= 0) {
                switch (finalControlsStatus >> 1) {
                case 0:
                    controlMask |= 0x100;   // up-right
                    break;
                case 1:
                    controlMask |= 0x80;    // down-right
                    break;
                case 2:
                    controlMask |= 0x200;   // down-left
                    break;
                default:
                    controlMask |= 0x40;    // up-left
                }
            }

            controlMask &= activeEntry->controlMask;

            if (activeEntry->controlMask == -1 || controlMask) {
                stopTitleSong();

                controlsStatus = controlMask;

                save68kRegisters();
                auto func = activeEntry->onSelect;
                SAFE_INVOKE(func);
                restore68kRegisters();
            }
        }

        // if no fire but there's movement, move to next entry
        if (!fire && finalControlsStatus >= 0) {
            static const size_t nextDirectionOffsets[4] = { 2, 1, 3, 0, };

            // will hold offset of next entry to move to (direction)
            int nextEntryDirection = nextDirectionOffsets[(finalControlsStatus >> 1) & 3];
            auto nextEntryIndex = (&activeEntry->leftEntry)[nextEntryDirection];

            nextEntry = findNextEntry(nextEntryIndex, nextEntryDirection);
        }
    } else if (finalControlsStatus < 0) {
        return;
    } else if (previousMenuItem) {
        currentMenu->selectedEntry = previousMenuItem;
        nextEntry = previousMenuItem;
    }

    if (nextEntry) {
        previousMenuItem = currentMenu->selectedEntry;
        currentMenu->selectedEntry = nextEntry;
        nextEntry->type1 == NO_BACKGROUND ? DrawMenu() : DrawMenuItem();
    }
}
