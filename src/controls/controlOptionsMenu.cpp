#include "controlOptionsMenu.h"
#include "controls.h"
#include "keyboard.h"
#include "joypads.h"
#include "menuMouse.h"
#include "text.h"
#include "setupKeyboardMenu.h"
#include "joypadConfigMenu.h"

static int16_t m_autoConnectJoypads;
static int16_t m_disableMenuControllers;

#include "controlOptions.mnu.h"

using namespace ControlOptionsMenu;

static int m_pl1ControlsOffset;
static int m_pl2ControlsOffset;

static std::pair<word, word> m_pl1EntryControls[kNumControlEntries];
static std::pair<word, word> m_pl2EntryControls[kNumControlEntries];

void showControlOptionsMenu()
{
    showMenu(controlOptionsMenu);
}

static void setupMouseWheelScrolling()
{
    setMouseWheelEntries({
        { pl1Control0, pl1Control0 + kNumControlEntries - 1, pl1ScrollUp, pl1ScrollDown },
        { pl2Control0, pl2Control0 + kNumControlEntries - 1, pl2ScrollUp, pl2ScrollDown },
    });
}

static void controlOptionsMenuInit()
{
    setupMouseWheelScrolling();

    m_autoConnectJoypads = getAutoConnectJoypads();
    m_disableMenuControllers = getDisableMenuControllers();
}

static void setupPlayerJoypadControlEntries(decltype(m_pl1EntryControls)& plEntryControls, MenuEntry *baseEntry, int& entriesFilled,
    int numControlEntries, int scrollOffset, int numJoypads, int initiallyVisibleJoypadEntries)
{
    int startJoypad = std::max(0, scrollOffset - numControlEntries + initiallyVisibleJoypadEntries);
    assert(!numJoypads || numJoypads == 1 || startJoypad + numControlEntries - entriesFilled <= numJoypads);

    int numJoypadEntries = std::min(numControlEntries - entriesFilled, numJoypads);
    assert(numJoypadEntries <= numControlEntries);

    for (int i = startJoypad; i < startJoypad + numJoypadEntries; i++) {
        tryReopeningJoypad(i);
        auto name = joypadName(i);
        copyStringToEntry(baseEntry[entriesFilled], name);

        baseEntry[entriesFilled].setBackgroundColor(kLightBlue);
        plEntryControls[entriesFilled].first = kJoypad;
        plEntryControls[entriesFilled].second = i;
        entriesFilled++;
    }
}

static void setupPlayerControlEntries(PlayerNumber player, decltype(m_pl1EntryControls)& plEntryControls,
    Entries baseControlsEntryIndex, Entries scrollUp, Entries scrollDown, int& scrollOffset, int initiallyVisibleJoypadEntries)
{
    using namespace SwosVM;

    assert(player == kPlayer1 || player == kPlayer2);

    static const char aKeyboard[] = "KEYBOARD";
    static const char aMouse[] = "MOUSE";
    static const char aNone[] = "NONE";

    int numJoypads = getNumJoypads();

    bool showScrollArrows = numJoypads > (player == kPlayer1 ? 2 : 1);
    setEntriesVisibility({ scrollUp, scrollDown }, showScrollArrows);

    if (!showScrollArrows || scrollOffset > numJoypads - initiallyVisibleJoypadEntries)
        scrollOffset = 0;

    auto baseEntry = getMenuEntry(baseControlsEntryIndex);

    const char *kFixedEntries[] = { cacheString(aNone), cacheString(aKeyboard), cacheString(aMouse) };
    static const Controls kFixedEntryControls[] = { kNone, kKeyboard1, kMouse };
    int startIndex = player == kPlayer1 ? 1 : 0;

    int entriesFilled = 0;

    for (int i = startIndex + scrollOffset; i < static_cast<int>(std::size(kFixedEntries)); i++) {
        strcpy(baseEntry[entriesFilled].string(), kFixedEntries[i]);
        baseEntry[entriesFilled].bg.entryColor = kLightBlue;
        plEntryControls[entriesFilled].first = kFixedEntryControls[i];

        if (kFixedEntryControls[i] == kKeyboard1 && player == kPlayer2)
            plEntryControls[entriesFilled].first = kKeyboard2;

        entriesFilled++;
    }

    setupPlayerJoypadControlEntries(plEntryControls, baseEntry, entriesFilled, kNumControlEntries, scrollOffset, numJoypads, initiallyVisibleJoypadEntries);

    assert(entriesFilled <= kNumControlEntries);

    while (entriesFilled < kNumControlEntries)
        baseEntry[entriesFilled++].hide();

    assert(entriesFilled == kNumControlEntries);
}

static void highlightSelectedControlEntries(decltype(m_pl1EntryControls) entry, Entries baseEntry, Controls plControls, int joypadIndex)
{
    assert(plControls != kJoypad || joypadIndex != kNoJoypad);

    for (int i = 0; i < kNumControlEntries; i++) {
        auto entryControls = entry[i].first;
        auto entryJoypadIndex = entry[i].second;

        if (plControls == entryControls && (entryControls != kJoypad || entryJoypadIndex == joypadIndex))
            getMenuEntry(baseEntry + i)->bg.entryColor = kPurple;
    }
}

static void enableControlConfigButtons(Entries redefineEntryIndex, Entries selectJoypadEntryIndex, Controls controls)
{
    auto numJoypads = getNumJoypads();

    auto redefineEntry = getMenuEntry(redefineEntryIndex);
    auto selectJoypadEntry = getMenuEntry(selectJoypadEntryIndex);

    if (controls == kJoypad || controls == kKeyboard1 || controls == kKeyboard2) {
        redefineEntry->bg.entryColor = kGreen;
        redefineEntry->disabled = 0;
    } else {
        redefineEntry->bg.entryColor = kGray;
        redefineEntry->disabled = 1;
    }

    if (numJoypads) {
        selectJoypadEntry->bg.entryColor = kGreen;
        selectJoypadEntry->disabled = 0;
    } else {
        selectJoypadEntry->bg.entryColor = kGray;
        selectJoypadEntry->disabled = 1;
    }
}

static void controlOptionsMenuOnDraw()
{
    setupPlayerControlEntries(kPlayer1, m_pl1EntryControls, pl1Control0, pl1ScrollUp, pl1ScrollDown, m_pl1ControlsOffset, 2);
    setupPlayerControlEntries(kPlayer2, m_pl2EntryControls, pl2Control0, pl2ScrollUp, pl2ScrollDown, m_pl2ControlsOffset, 1);

    highlightSelectedControlEntries(m_pl1EntryControls, pl1Control0, getPl1Controls(), getPl1JoypadIndex());
    highlightSelectedControlEntries(m_pl2EntryControls, pl2Control0, getPl2Controls(), getPl2JoypadIndex());

    enableControlConfigButtons(pl1RedefineControls, pl1SelectJoypad, getPl1Controls());
    enableControlConfigButtons(pl2RedefineControls, pl2SelectJoypad, getPl2Controls());
}

static void pl1SelectKeyboard()
{
    assert(getPl1Controls() != kKeyboard1 && getPl1Controls() != kKeyboard2);

    logInfo("Player 1 keyboard controls selected");

    setPl1Controls(kKeyboard1);

    if (!pl1HasBasicBindings()) {
        logInfo("Player 1 keyboard controls don't include basic controls, redefining...");
        redefinePlayer1Controls();
    }
}

static void pl1SelectMouse()
{
    assert(getPl1Controls() != kMouse);

    logInfo("Player 1 mouse controls selected");

    setPl1Controls(kMouse);

    waitForKeyboardAndMouseIdle();

    if (getPl2Controls() == kMouse)
        setPl2Controls(kNone);
}

static std::tuple<MenuEntry *, int, int, int> getControlsInfo(int baseEntryIndex, decltype(m_pl1EntryControls) plControls)
{
    auto entry = A5.as<MenuEntry *>();
    assert(entry->ordinal >= baseEntryIndex && entry->ordinal < baseEntryIndex + kNumUniqueControls);

    int index = entry->ordinal - baseEntryIndex;
    int newControls = plControls[index].first;
    int joypadNo = plControls[index].second;

    return std::make_tuple(entry, index, newControls, joypadNo);
}

static void pl1SelectControls()
{
    MenuEntry *entry;
    int index, newControls, joypadNo;

    std::tie(entry, index, newControls, joypadNo) = getControlsInfo(pl1Control0, m_pl1EntryControls);

    assert(newControls != kJoypad || joypadNo >= 0);

    if (getPl1Controls() != newControls || getPl1Controls() == kJoypad && getPl1JoypadIndex() != joypadNo) {
        switch (newControls) {
        case kKeyboard1:
            pl1SelectKeyboard();
            break;
        case kMouse:
            pl1SelectMouse();
            break;
        case kJoypad:
            selectJoypadControls(kPlayer1, joypadNo);
            break;
        default:
            assert(false);
        }
    }
}

static void pl2SelectNoControls()
{
    assert(getPl2Controls() != kNone);

    logInfo("Player 2 controls disabled");

    setPl2Controls(kNone);
}

static void pl2SelectKeyboard()
{
    assert(getPl2Controls() != kKeyboard1 && getPl2Controls() != kKeyboard2);

    logInfo("Player 2 keyboard controls selected");

    setPl2Controls(kKeyboard2);

    if (!pl2HasBasicBindings()) {
        logInfo("Player 2 keyboard controls don't include basic controls, redefining...");
        redefinePlayer2Controls();
    }
}

static void pl2SelectMouse()
{
    assert(getPl2Controls() != kMouse);

    logInfo("Player 2 mouse controls selected");

    setPl2Controls(kMouse);

    if (getPl1Controls() == kMouse)
        pl1SelectKeyboard();
}

static void pl2SelectControls()
{
    MenuEntry *entry;
    int index, newControls, joypadNo;

    std::tie(entry, index, newControls, joypadNo) = getControlsInfo(pl2Control0, m_pl2EntryControls);

    if (getPl2Controls() != newControls || getPl2Controls() == kJoypad && getPl2Controls() != joypadNo) {
        switch (newControls) {
        case kNone:
            pl2SelectNoControls();
            break;
        case kKeyboard2:
            pl2SelectKeyboard();
            break;
        case kMouse:
            pl2SelectMouse();
            break;
        case kJoypad:
            selectJoypadControls(kPlayer2, joypadNo);
            break;
        default:
            assert(false);
        }
    }
}

static void toggleAutoConnectJoypads()
{
    m_autoConnectJoypads = !m_autoConnectJoypads;
    setAutoConnectJoypads(m_autoConnectJoypads != 0);
    logInfo("Auto-connect joypads is changed to %s", m_autoConnectJoypads ? "on" : "off");
}

static void toggleMenuControllers()
{
    m_disableMenuControllers = !m_disableMenuControllers;
    setDisableMenuControllers(m_disableMenuControllers != 0);
    logInfo("Game controllers in menus %s", m_disableMenuControllers ? "disabled" : "enabled");
}

static void exitControlsMenu()
{
    m_pl1ControlsOffset = 0;
    m_pl2ControlsOffset = 0;
    SetExitMenuFlag();
}

static void pl1ScrollUpSelected()
{
    m_pl1ControlsOffset = std::max(0, m_pl1ControlsOffset - 1);
}

static void pl1ScrollDownSelected()
{
    int maxOffset = std::max(2 + getNumJoypads() - kNumUniqueControls, 0);
    m_pl1ControlsOffset = std::min(m_pl1ControlsOffset + 1, maxOffset);
}

static void pl2ScrollUpSelected()
{
    m_pl2ControlsOffset = std::max(0, m_pl2ControlsOffset - 1);
}

static void pl2ScrollDownSelected()
{
    int maxOffset = std::max(3 + getNumJoypads() - kNumUniqueControls, 0);
    m_pl2ControlsOffset = std::min(m_pl2ControlsOffset + 1, maxOffset);
}

static bool selectJoypadWithButtonPress(PlayerNumber player)
{
    using namespace SwosVM;

    assert(player == kPlayer1 || player == kPlayer2);

    if (getNumJoypads() < 2)
        return false;

    redrawMenuBackground();

    constexpr int kPromptX = kMenuScreenWidth / 2;
    constexpr int kPromptY = 40;
    constexpr int kAbortY = 150;

    drawMenuTextCentered(kPromptX, kPromptY, cacheString("PRESS ANY CONTROLLER BUTTON TO SELECT"));

    constexpr char aGameControllerForPlayer[] = "GAME CONTROLLER FOR PLAYER 1";
    auto text = cacheString(aGameControllerForPlayer);
    text[sizeof(aGameControllerForPlayer) - 2] = player == kPlayer1 ? '1' : '2';

    drawMenuTextCentered(kPromptX, kPromptY + 10, text);
    drawMenuTextCentered(kPromptX, kAbortY, cacheString("ESCAPE/MOUSE CLICK ABORTS"));

    SWOS::FlipInMenu();

    bool changed = false;

    waitForJoypadButtonsIdle();

    while (true) {
        processControlEvents();
        SWOS::GetKey();

        if (SDL_GetMouseState(nullptr, nullptr) || swos.lastKey == kKeyEscape)
            break;

        auto joypadIndex = getJoypadWithButtonDown();

        if (joypadIndex >= 0) {
            auto currentControls = player == kPlayer1 ? getPl1Controls() : getPl2Controls();
            auto currentJoypadIndex = player == kPlayer1 ? getPl1JoypadIndex() : getPl2JoypadIndex();

            if (currentControls == kJoypad && currentJoypadIndex == joypadIndex) {
                changed = true;
            } else {
                logInfo("Selecting joypad %d for player %d via button press", joypadIndex, player == kPlayer1 ? 1 : 2);
                changed = selectJoypadControls(player, joypadIndex);
            }

            break;
        }

        SDL_Delay(25);
    }

    if (changed) {
        auto currentJoypadIndex = player == kPlayer1 ? getPl1JoypadIndex() : getPl2JoypadIndex();
        auto name = joypadName(currentJoypadIndex);

        char buf[128];
        snprintf(buf, sizeof(buf), "%s SELECTED", name ? name : "<UNKNOWN CONTROLLER>");

        drawMenuTextCentered(kPromptX, (kPromptY + kAbortY) / 2, buf, -1, kYellowText);
        SWOS::FlipInMenu();
        SDL_Delay(600);
    }

    waitForKeyboardAndMouseIdle();
    waitForJoypadButtonsIdle();

    return changed;
}

static void scrollJoypadIntoView(PlayerNumber player)
{
    assert(player == kPlayer1 || player == kPlayer2);

    auto joypadIndex = player == kPlayer1 ? getPl1JoypadIndex() : getPl2JoypadIndex();
    assert(joypadIndex >= 0 && joypadIndex < SDL_NumJoysticks());

    auto minOffset = std::max(0, joypadIndex - (player == kPlayer1));
    auto maxOffset = joypadIndex + 2 + (player == kPlayer2);

    auto& scrollOffset = player == kPlayer1 ? m_pl1ControlsOffset : m_pl2ControlsOffset;
    if (scrollOffset < minOffset)
        scrollOffset = minOffset;
    if (scrollOffset > maxOffset)
        scrollOffset = maxOffset;
}

static void pl1SelectWhichJoypad()
{
    if (selectJoypadWithButtonPress(kPlayer1))
        scrollJoypadIntoView(kPlayer1);
}

static void pl2SelectWhichJoypad()
{
    if (selectJoypadWithButtonPress(kPlayer2))
        scrollJoypadIntoView(kPlayer2);
}

static void redefinePlayer1Controls()
{
    if (getPl1Controls() == kKeyboard1)
        showSetupKeyboardMenu(kPlayer1);
    else if (getPl1Controls() == kJoypad)
        showJoypadConfigMenu(kPlayer1, getPl1JoypadIndex());
}

static void redefinePlayer2Controls()
{
    if (getPl2Controls() == kKeyboard2)
        showSetupKeyboardMenu(kPlayer2);
    else if (getPl2Controls() == kJoypad)
        showJoypadConfigMenu(kPlayer2, getPl2JoypadIndex());
}
