#include "selectMatchControls.h"
#include "menuMouse.h"
#include "controls.h"
#include "joypads.h"
#include "keyboard.h"
#include "setupKeyboardMenu.h"
#include "joypadConfigMenu.h"
#include "selectMatchControls.mnu.h"

constexpr int kKeyboardOffset = 0;
constexpr int kMouseOffset = 1;
constexpr int kJoypadOffset = 2;

static bool m_success;
static int m_controlsOffset;
static bool m_twoPlayers;

static bool m_blockControlDrag;

using namespace SelectMatchControlsMenu;

static int m_joypadIndices[kNumControlEntries - kJoypadOffset];

static int numPlayers()
{
    return (swos.playMatchTeam1Ptr->teamControls != kComputerTeam) + (swos.playMatchTeam2Ptr->teamControls != kComputerTeam);
}

static bool isFinalTeamSetup()
{
    if (numPlayers() == 0)
        return false;

    return !swos.isFirstPlayerChoosingTeam || numPlayers() < 2;
}

void SWOS::PlayMatchSelected()
{
    if (reinterpret_cast<TeamFile *>(swos.careerTeam)->teamControls == kComputerTeam)
        swos.careerTeamSetupDone = 1;

    if (isFinalTeamSetup() && !showSelectMatchControlsMenu())
        return;

    swos.playerNumThatStarted = numPlayers();
    SetExitMenuFlag();
}

bool showSelectMatchControlsMenu()
{
    assert(swos.playMatchTeam1Ptr && swos.playMatchTeam2Ptr);

    m_success = false;
    m_twoPlayers = numPlayers() > 1;

    showMenu(selectMatchControlsMenu);

    return m_success;
}

static void initMenu();
static void rearrangeEntriesForSinglePlayer();
static void setupMouseWheelScrolling();
static void updateMenu();
static void updateTeamNames();
static void updateCurrentControls();
static void updateControlList();
static void updateScrollButtons();
static void updateConfigureButtons();
static void updateWarnings();
static void fixSelection();

static void selectMatchControlsMenuOnInit()
{
    initMenu();
    updateMenu();
}

static void selectMatchControlsMenuOnRestore()
{
    initMenu();
    updateMenu();
}

static void updateDragBlock()
{
    bool mouseDragging = m_blockControlDrag && (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK);
    bool keyboardFireOrMove = (swos.controlMask & (kLeftMask | kRightMask | kFireMask | kShortFireMask)) != 0;

    if (!keyboardFireOrMove && !mouseDragging)
        m_blockControlDrag = false;
}

static void selectMatchControlsMenuOnDraw()
{
    updateDragBlock();
    updateMenu();
}

static void selectControl()
{
    if (m_blockControlDrag)
        return;

    auto entry = A5.asMenuEntry();

    assert(entry->ordinal >= firstControl && entry->ordinal <= lastControl);

    if (swos.controlMask & (kFireMask | kShortFireMask)) {
        int offset = entry->ordinal - firstControl;

        if (!m_twoPlayers || (swos.controlMask & (kLeftMask | kRightMask))) {
            bool setPlayer1 = !m_twoPlayers || (swos.controlMask & kLeftMask);
            auto player = setPlayer1 ? kPlayer1 : kPlayer2;

            switch (offset) {
            case kKeyboardOffset:
                {
                    auto keyboard = setPlayer1 ? kKeyboard1 : kKeyboard2;
                    setControls(player, keyboard);
                }
                break;
            case kMouseOffset:
                setControls(player, kMouse);
                break;
            default:
                {
                    int joypadIndex = entry->ordinal - firstControl - kJoypadOffset;
                    joypadIndex = m_joypadIndices[joypadIndex];
                    setControls(player, kJoypad, joypadIndex);
                }
                break;
            }

            m_blockControlDrag = true;
        }
    }
}

static void player1ControlsSelected()
{
    if (!m_blockControlDrag && (swos.controlMask & (kFireMask | kShortFireMask)))
        setPl1Controls(kNone);

    m_blockControlDrag = true;
}

static void player2ControlsSelected()
{
    assert(m_twoPlayers);

    if (!m_blockControlDrag && (swos.controlMask & (kFireMask | kShortFireMask)))
        setPl2Controls(kNone);

    m_blockControlDrag = true;
}

static void cancelSelection()
{
    m_success = false;
    SetExitMenuFlag();
}

static void confirmSelection()
{
    m_success = true;
    SetExitMenuFlag();
}

static void configurePl1Controls()
{
    if (getPl1Controls() == kKeyboard1)
        showSetupKeyboardMenu(kPlayer1);
    else if (getPl1Controls() == kJoypad)
        showJoypadConfigMenu(kPlayer1, getPl1JoypadIndex());
    else
        assert(false);
}

static void configurePl2Controls()
{
    assert(m_twoPlayers);

    if (getPl2Controls() == kKeyboard2)
        showSetupKeyboardMenu(kPlayer2);
    else if (getPl2Controls() == kJoypad)
        showJoypadConfigMenu(kPlayer2, getPl2JoypadIndex());
    else
        assert(false);
}

static void scrollUpSelected()
{
    if (m_controlsOffset > 0) {
        m_controlsOffset--;
        updateControlList();
    }
}

static int numControls()
{
    int numJoypadPlayers = (getPl1Controls() == kJoypad) + (m_twoPlayers && getPl2Controls() == kJoypad);
    return getNumJoypads() - numJoypadPlayers + kJoypadOffset;
}

static void scrollDownSelected()
{
    int maxScrollOffset = numControls() - kNumControlEntries;

    if (m_controlsOffset < maxScrollOffset) {
        m_controlsOffset++;
        updateControlList();
    }
}

static void initMenu()
{
    updateTeamNames();
    rearrangeEntriesForSinglePlayer();
    setupMouseWheelScrolling();
}

static void rearrangeEntriesForSinglePlayer()
{
    if (!m_twoPlayers) {
        constexpr int kColumn1Width = 153;
        constexpr int kColumn2Width = 152;
        static_assert(3 * kGapWidth + kColumn1Width + kColumn2Width == kMenuScreenWidth, "Yarrr");

        for (auto entry : { &team1Entry, &player1ControlsEntry, &player1WarningEntry, &configure1Entry })
            entry->width = kColumn1Width;

        bool gotScroller = numControls() > kNumControlEntries;

        int column2Width = kColumn2Width - (gotScroller ? scrollUpEntry.width : 0);

        for (auto entry = &controlsLabelEntry; entry <= &lastControlEntry; entry++) {
            entry->x = 2 * kGapWidth + kColumn1Width;
            entry->width = column2Width;

            if (gotScroller && entry < &secondLastControlEntry)
                entry->rightEntry = scrollUp;
        }

        scrollUpEntry.x = controlsLabelEntry.endX() + 1;
        scrollDownEntry.x = scrollUpEntry.x;

        playEntry.x = controlsLabelEntry.x;
        playEntry.width = (column2Width - kBottomButtonsColumnGap) / 2;
        backEntry.x = playEntry.endX() + kBottomButtonsColumnGap;
        backEntry.width = playEntry.width;
    }
}

static void setupMouseWheelScrolling()
{
    setMouseWheelEntries({{ firstControl, firstControl + kNumControlEntries - 1, scrollUp, scrollDown }});
}

static void updateMenu()
{
    updateControlList();
    updateScrollButtons();
    updateCurrentControls();
    updateConfigureButtons();
    updateWarnings();
    fixSelection();
}

static void updateTeamNames()
{
    copyStringToEntry(team1Entry, swos.playMatchTeam1Ptr->teamName);
    if (m_twoPlayers)
        copyStringToEntry(team2Entry, swos.playMatchTeam2Ptr->teamName);
    else
        team2Entry.hide();
}

static const char *controlsToString(Controls controls, int joypadIndex)
{
    switch (controls) {
    case kNone: return "NONE";
    case kKeyboard1:
    case kKeyboard2: return "KEYBOARD";
    case kMouse: return "MOUSE";
    case kJoypad: return joypadName(joypadIndex);
    default: return "";
    }
}

static void updateCurrentControls()
{
    auto pl1Controls = getPl1Controls();
    if (pl1Controls != kNone) {
        player1ControlsEntry.show();
        auto controlString = controlsToString(pl1Controls, getPl1JoypadIndex());
        player1ControlsEntry.copyString(controlString);
    } else {
        player1ControlsEntry.hide();
    }

    auto pl2Controls = getPl2Controls();
    if (m_twoPlayers && pl2Controls != kNone) {
        player2ControlsEntry.show();
        auto controlString = controlsToString(pl2Controls, getPl2JoypadIndex());
        player2ControlsEntry.copyString(controlString);
    } else {
        player2ControlsEntry.hide();
    }
}

static void updateControlList()
{
    auto entry = &firstControlEntry;
    auto sentinel = entry + kNumControlEntries;

    assert(kNumControlEntries > kJoypadOffset && (numControls() > kNumControlEntries || !m_controlsOffset));

    if (m_controlsOffset <= kKeyboardOffset)
        entry++->copyString("KEYBOARD");
    if (m_controlsOffset <= kMouseOffset)
        entry++->copyString("MOUSE");

    int initialJoypadIndex = std::max(0, m_controlsOffset - kJoypadOffset);
    int storedJoypadIndex = 0;

    for (int joypadIndex = initialJoypadIndex; joypadIndex < getNumJoypads() && entry < sentinel; joypadIndex++) {
        if (joypadIndex != getPl1JoypadIndex() && (!m_twoPlayers || joypadIndex != getPl2JoypadIndex())) {
            entry->show();
            entry++->copyString(joypadName(joypadIndex));
            m_joypadIndices[storedJoypadIndex++] = joypadIndex;
        }
    }

    while (entry < sentinel)
        entry++->hide();
}

static void updateScrollButtons()
{
    static const std::vector<int> kScrollButtons = { scrollDown, scrollUp };

    if (numControls() <= kNumControlEntries) {
        m_controlsOffset = 0;
        setEntriesVisibility(kScrollButtons, false);
    } else {
        setEntriesVisibility(kScrollButtons, true);
    }
}

static void setEntryDisabled(MenuEntry& entry, bool disabled)
{
    if (disabled) {
        entry.disable();
        entry.setBackgroundColor(kDisabledColor);
    } else {
        entry.enable();
        entry.setBackgroundColor(kEnabledColor);
    }
}

void updateConfigureButtons()
{
    bool configure1Disabled = getPl1Controls() == kNone || getPl1Controls() == kMouse;
    setEntryDisabled(configure1Entry, configure1Disabled);

    if (m_twoPlayers) {
        bool configure2Disabled = getPl2Controls() == kNone || getPl2Controls() == kMouse;
        setEntryDisabled(configure2Entry, configure2Disabled);
    } else {
        configure2Entry.hide();
    }
}

static bool hasBasicEvents(Controls controls, int joypadIndex)
{
    switch (controls) {
    case kKeyboard1:
        return pl1HasBasicBindings();
    case kKeyboard2:
        return pl2HasBasicBindings();
    case kMouse:
    case kNone:
        return true;
    case kJoypad:
        return joypadHasBasicBindings(joypadIndex);
    default:
        assert(false);
        return false;
    }
}

static void updateWarnings()
{
    if (!hasBasicEvents(getPl1Controls(), getPl1JoypadIndex())) {
        player1WarningEntry.show();
        player1ControlsEntry.setFrameColor(kYellow);
    } else {
        player1ControlsEntry.setFrameColor(kNoFrame);
        player1WarningEntry.hide();
    }

    if (m_twoPlayers && !hasBasicEvents(getPl2Controls(), getPl2JoypadIndex())) {
        player2WarningEntry.show();
        player2ControlsEntry.setFrameColor(kYellow);
    } else {
        player2ControlsEntry.setFrameColor(kNoFrame);
        player2WarningEntry.hide();
    }
}

void fixSelection()
{
    auto& selectedEntry = getCurrentMenu()->selectedEntry;

    if (!selectedEntry->selectable()) {
        if (selectedEntry->ordinal == player1Controls || selectedEntry->ordinal == player2Controls)
            selectedEntry = &firstControlEntry;

        while (!selectedEntry->selectable() && selectedEntry->ordinal >= firstControl)
            selectedEntry--;
    }

    if (!selectedEntry->selectable())
        selectedEntry = &playEntry;
}
