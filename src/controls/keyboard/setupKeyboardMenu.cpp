#include "setupKeyboardMenu.h"
#include "keyboard.h"
#include "text.h"
#include "keyBuffer.h"
#include "menuMouse.h"
#include "selectGameControlEventsMenu.h"
#include "redefineKeysMenu.h"
#include "testKeyboardMenu.h"
#include "setupKeyboard.mnu.h"

using namespace SetupKeyboardMenu;

static PlayerNumber m_player;

static int m_scrollOffset;
static int m_numBindings;

void showSetupKeyboardMenu(PlayerNumber player)
{
    m_player = player;
    showMenu(setupKeyboardMenu);
}

void setupMouseWheelScrolling()
{
    setMouseWheelEntries({
        { firstKey, firstKey + kMaxKeyEntriesShown - 1, leftUpArrow, leftDownArrow },
        { firstAction, firstAction + kMaxKeyEntriesShown - 1, rightUpArrow, rightDownArrow },
    });
}

static void setupKeyboardMenuOnInit()
{
    m_scrollOffset = 0;

    setupMouseWheelScrolling();
    setupKeyboardMenuOnRestore();
}

static void updateScrollArrowsVisibility(bool show)
{
    setEntriesVisibility({ leftUpArrow, leftDownArrow, rightUpArrow, rightDownArrow }, show);
}

static void normalizeScrollOffset()
{
    if (m_numBindings <= kMaxKeyEntriesShown)
        m_scrollOffset = 0;
    else
        m_scrollOffset = std::min<int>(m_numBindings - kMaxKeyEntriesShown, m_scrollOffset);
}

static void updateKeyAndActionEntriesVisibility()
{
    auto keyEntry = getMenuEntry(firstKey);
    auto keyLegendEntry = getMenuEntry(keyLegend);
    auto actionEntry = getMenuEntry(firstAction);
    auto actionLegendEntry = getMenuEntry(actionLegend);

    auto numVisibleEntries = std::min(m_numBindings, kMaxKeyEntriesShown);

    while (numVisibleEntries--) {
        keyEntry++->show();
        actionEntry++->show();
    }

    if (keyEntry->ordinal != firstKey) {
        keyLegendEntry->y = keyEntry[-1].endY() + kVerticalGap;
        actionLegendEntry->y = actionEntry[-1].endY() + kVerticalGap;
    } else {
        keyLegendEntry->hide();
        actionLegendEntry->hide();
    }

    while (keyEntry->ordinal <= lastKey) {
        keyEntry++->hide();
        actionEntry++->hide();
    }
}

static void fillKeyAndActionEntries()
{
    const auto& bindings = getPlayerKeyBindings(m_player);
    m_numBindings = bindings.size();

    bool showScrollingArrows = m_numBindings > kMaxKeyEntriesShown;
    updateScrollArrowsVisibility(showScrollingArrows);

    normalizeScrollOffset();
    updateKeyAndActionEntriesVisibility();

    int numEntries = std::min(m_numBindings, kMaxKeyEntriesShown);

    auto keyEntry = getMenuEntry(firstKey);
    auto actionEntry = getMenuEntry(firstAction);

    for (int i = m_scrollOffset; i < m_scrollOffset + numEntries; i++, keyEntry++, actionEntry++) {
        auto keyName = scancodeToString(bindings[i].key);
        copyStringToEntry(*keyEntry, keyName);
        gameControlEventToString(bindings[i].events, actionEntry->fg.string, kStdMenuTextSize);
    }
}

static void fixSelectedEntry()
{
    auto& selectedEntry = getCurrentMenu()->selectedEntry;

    while (selectedEntry->ordinal > 0 && isEntryReachable(selectedEntry->ordinal)) {
        if (selectedEntry->selectable())
            return;

        selectedEntry--;
    }

    highlightEntry(SetupKeyboardMenu::exit);
}

static void updateMenu()
{
    fillKeyAndActionEntries();
    fixSelectedEntry();
}

static void setupKeyboardMenuOnRestore()
{
    updateMenu();
}

static void setupKeyboardMenuOnDraw()
{
    if (!m_numBindings) {
        auto tempBuf = getMenuTempBuffer();
        tempBuf[0] = '/';
        tempBuf[1] = '\0';

        auto firstKeyEntry = getMenuEntry(firstKey);
        auto firstActionEntry = getMenuEntry(firstAction);

        drawMenuTextCentered(firstKeyEntry->centerX(), firstKeyEntry->y, tempBuf);
        drawMenuTextCentered(firstActionEntry->centerX(), firstActionEntry->y, tempBuf);
    }
}

static void onScrollUp()
{
    if (m_scrollOffset > 0) {
        m_scrollOffset--;
        updateMenu();
    }
}

static void onScrollDown()
{
    if (m_scrollOffset < m_numBindings - kMaxKeyEntriesShown) {
        m_scrollOffset++;
        updateMenu();
    }
}

static bool confirmDeletion(SDL_Scancode key)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "TO DELETE '%s' KEY?", scancodeToString(key));
    return showContinueAbortPrompt("DELETE KEY", swos.aOk, swos.aCancel, { "ARE YOU SURE YOU WANT", buf });
}

static void deleteKey()
{
    auto entry = A5.asMenuEntry();
    assert(entry->selectable());

    size_t keyIndex = entry->ordinal - firstKey + m_scrollOffset;
    auto binding = getPlayerKeyBindingAt(m_player, keyIndex);

    if (confirmDeletion(binding.key)) {
        deleteKeyBindingAt(m_player, keyIndex);
        updateMenu();
    }
}

static void updateAction()
{
    auto entry = A5.asMenuEntry();
    assert(entry->selectable());

    size_t keyIndex = entry->ordinal - firstAction + m_scrollOffset;

    auto fillKeyName = [](char *buf, int bufSize, SDL_Scancode key) {
        auto keyName = scancodeToString(key);
        snprintf(buf, bufSize, "FOR KEY: '%s'", keyName);
    };

    auto setEvents = [](int index, GameControlEvents events, bool) {
        updatePlayerKeyBindingEventsAt(m_player, index, events);
    };
    auto getEvents = [fillKeyName](int& index, char *buf, int bufSize) {
        index = std::min(m_numBindings - 1, index);
        auto binding = getPlayerKeyBindingAt(m_player, index);
        fillKeyName(buf, bufSize, binding.key);
        return std::make_pair(getPlayerKeyBindingAt(m_player, index).events, false);
    };

    updateGameControlEvents(keyIndex, setEvents, getEvents);
    updateMenu();
}

static void drawInputKeyPrompt()
{
    constexpr int kPressKeyYLine1 = 30;
    constexpr int kPressKeyYLine2 = 40;

    redrawMenuBackground();
    drawMenuTextCentered(kMenuScreenWidth / 2, kPressKeyYLine1, "PRESS A KEY ON THE KEYBOARD");
    drawMenuTextCentered(kMenuScreenWidth / 2, kPressKeyYLine2, "(MOUSE CLICK CANCELS)");

    SWOS::Flip();
}

static void addKeyMapping()
{
    constexpr int kWarningY = 60;

    drawInputKeyPrompt();
    auto key = getControlKey(m_player, kWarningY);

    if (key != SDL_SCANCODE_UNKNOWN) {
        auto keyName = scancodeToString(key);
        char buf[kStdMenuTextSize];
        snprintf(buf, sizeof(buf), "FOR KEY: '%s'", keyName);

        auto events = getNewGameControlEvents(buf);

        if (events.first) {
            addPlayerKeyBinding(m_player, key, events.second);
            setupKeyboardMenuOnRestore();
        }
    }
}

static void redefineKeys()
{
    auto result = promptForDefaultKeys(m_player);
    if (result.first) {
        setDefaultKeyPackForPlayer(m_player, result.second);
        updateMenu();
    }
}

static void testKeyboard()
{
    showTestKeyboardMenu(m_player);
}
