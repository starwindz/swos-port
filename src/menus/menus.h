#pragma once

#include "menuCodes.h"
#include "util.h"
#include "render.h"

constexpr int kMenuStringLength = 70;
constexpr int kMenuOffset = 8;

static inline void showError(const char *error)
{
    A0 = error;
    ShowErrorMenu();
}

static inline Menu *getCurrentMenu()
{
    return reinterpret_cast<Menu *>(swos.g_currentMenu);
}

static inline MenuEntry *getMenuEntry(int ordinal)
{
    assert(ordinal >= 0 && ordinal < 255);

    auto ptr = swos.g_currentMenu + sizeof(Menu) + ordinal * sizeof(MenuEntry);
    return reinterpret_cast<MenuEntry *>(ptr);
}

static inline int getCurrentEntryOrdinal()
{
    auto currentMenu = getCurrentMenu();
    return currentMenu->selectedEntry ? currentMenu->selectedEntry->ordinal : -1;
}

static inline void drawMenuSprite(int x, int y, int index)
{
    D0 = index;
    D1 = x;
    D2 = y;
    SAFE_INVOKE(DrawSprite);
}

static inline void redrawMenuBackground()
{
    memcpy(swos.linAdr384k, swos.linAdr384k + 128 * 1024, kVgaScreenSize);
}

static inline void redrawMenuBackground(int lineFrom, int lineTo)
{
    int offset = lineFrom * kMenuScreenWidth;
    int length = (lineTo - lineFrom) * kMenuScreenWidth;
    memcpy(swos.linAdr384k + offset, swos.linAdr384k + 128 * 1024 + offset, length);
}

static inline void highlightEntry(MenuEntry *entry)
{
    auto menu = getCurrentMenu();
    menu->selectedEntry = entry;
}

static inline void highlightEntry(int ordinal)
{
    auto entry = getMenuEntry(ordinal);
    highlightEntry(entry);
}

static inline void setCurrentEntry(int ordinal)
{
    highlightEntry(ordinal);
}

static inline bool inputText(char *destBuffer, int maxLength, bool allowExtraChars = false)
{
    A0 = destBuffer;
    D0 = maxLength;
    swos.g_allowExtraCharsFlag = allowExtraChars;

    SAFE_INVOKE(InputText);

    return D0.asWord() == 0;
}

static inline void copyStringToEntry(MenuEntry& entry, const char *str)
{
    entry.copyString(str);
}

static inline void copyStringToEntry(int entryIndex, const char *str)
{
    copyStringToEntry(*getMenuEntry(entryIndex), str);
}

static inline char *copyStringToMenuBuffer(const char *str)
{
    auto buf = getCurrentMenu()->endOfMenuPtr.asAlignedCharPtr();
    strcpy(buf, str);
    assert(buf + strlen(str) <= swos.g_currentMenu + sizeof(swos.g_currentMenu));
    return buf;
}

static inline char *getMenuTempBuffer()
{
    auto tempBuf = getCurrentMenu()->endOfMenuPtr.asAlignedCharPtr();
    assert(tempBuf + 256 < swos.g_currentMenu + sizeof(swos.g_currentMenu));
    return tempBuf;
}

void setEntriesVisibility(const std::vector<int>& entryIndices, bool visible);
void selectEntry(MenuEntry *entry, int controlMask = kShortFireMask);
void selectEntry(int ordinal);

void showMenu(const BaseMenu& menu);
void unpackMenu(const void *src, char *dst = swos.g_currentMenu);
void restoreMenu(const void *menu, int selectedEntry);
void activateMenu(const void *menu);
const void *getCurrentPackedMenu();

bool inputNumber(MenuEntry *entry, int maxDigits, int minNum, int maxNum);
bool showContinueAbortPrompt(const char *header, const char *continueText,
    const char *abortText, const std::vector<const char *>& messageLines);

char *menuAlloc(size_t size);
