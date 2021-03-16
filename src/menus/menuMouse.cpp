// Handles mouse movement and clicks in the menus.

#include "menuMouse.h"
#include "menus.h"
#include "menuControls.h"
#include "controls.h"

constexpr int kScrollRateMs = 100;

static Uint32 m_buttons;
static Uint32 m_previousButtons;

static int m_x;     // current mouse coordinates
static int m_y;
static int m_clickX;
static int m_clickY;

static Uint32 m_lastScrollTick;
static bool m_scrolling;

static MenuEntry *m_currentEntryUnderPointer;
static MenuEntry *m_scrollEntry;

struct MenuMouseWheelData {
    MouseWheelEntryGroupList entries;
    int upEntry = -1;
    int downEntry = -1;

    bool doesEntryRespondToMouseWheel(const MenuEntry& entry) const {
        auto it = std::find_if(entries.begin(), entries.end(), [&entry](const auto& wheelEntry) {
            return entry.ordinal >= wheelEntry.first && entry.ordinal <= wheelEntry.last;
        });

        return it != entries.end();
    }

    bool globalHandlerActive() const {
        return upEntry >= 0 || downEntry >= 0;
    }

    void resetGlobalHandler() {
        upEntry = downEntry = -1;
    }
};

// hold one initial entry for the main menu
static std::vector<MenuMouseWheelData> m_mouseWheelData = { {} };

using ReachabilityMap = std::array<bool, 256>;
static ReachabilityMap m_reachableEntries;

static std::tuple<bool, bool, bool> updateMouseStatus()
{
    m_buttons = SDL_GetMouseState(&m_x, &m_y);

    bool leftButtonDownNow = (m_buttons & SDL_BUTTON_LMASK) != 0;
    bool leftButtonDownPreviously = (m_previousButtons & SDL_BUTTON_LMASK) != 0;
    bool rightButtonDownNow = (m_buttons & SDL_BUTTON_RMASK) != 0;
    bool rightButtonDownPreviously = (m_previousButtons & SDL_BUTTON_RMASK) != 0;

    bool leftButtonJustPressed = leftButtonDownNow && !leftButtonDownPreviously;
    bool leftButtonJustReleased = !leftButtonDownNow && leftButtonDownPreviously;
    bool rightButtonJustReleased = !rightButtonDownNow && rightButtonDownPreviously;

    m_previousButtons = m_buttons;

    return { leftButtonJustPressed, leftButtonJustReleased, rightButtonJustReleased };
}

// Tries to recognize if the entry is a scroll arrow by using some heuristics.
static bool isEntryScroller(const MenuEntry& entry)
{
    assert(!m_mouseWheelData.empty());
    const auto& wheelData = m_mouseWheelData.back();

    if (entry.ordinal == wheelData.upEntry || entry.ordinal == wheelData.downEntry)
        return true;

    static const std::array<int, 4> kArrowSprites = { kUpArrowSprite, kDownArrowSprite, kLeftArrowSprite, kRightArrowSprite };
    if (entry.type == kEntrySprite2 && std::find(kArrowSprites.begin(), kArrowSprites.end(), entry.fg.spriteIndex) != kArrowSprites.end())
        return true;

    // also quicken +/- fields
    if (entry.isString() && (entry.string()[0] == '+' || entry.string()[0] == '-') && entry.string()[1] == '\0')
        return true;

    return false;
}

static bool handleClickScrolling()
{
    if (m_scrolling) {
        if (m_buttons & SDL_BUTTON_LMASK) {
            auto now = SDL_GetTicks();

            if (now >= m_lastScrollTick + kScrollRateMs) {
                assert(m_scrollEntry && isEntryScroller(*m_scrollEntry));
                selectEntry(m_scrollEntry);
                m_lastScrollTick = now;
            }

            return true;
        } else {
            m_scrolling = false;
            return true;
        }
    }

    return false;
}

static bool handleRightClickExitMenu(bool rightButtonJustReleased)
{
    if (rightButtonJustReleased) {
        swos.chosenTactics = -1;    // edit tactics menu
        swos.teamsType = -1;        // select files menu
        if (getCurrentPackedMenu() == &swos.diyCompetitionMenu)
            swos.diySelected = -1;
        ExitSelectTeams();
        if (swos.choosingPreset)    // preset competition must be aborted at any level
            swos.abortSelectTeams = -1;
        swos.g_exitGameFlag = -1;   // play match menu
        return true;
    }

    return false;
}

static bool mapCoordinatesToGameArea()
{
    int windowWidth, windowHeight;

    if (isInFullScreenMode()) {
        std::tie(windowWidth, windowHeight) = getFullScreenDimensions();
        m_x = m_x * kVgaWidth / windowWidth;
        m_y = m_y * kVgaHeight / windowHeight;
        return true;
    }

    std::tie(windowWidth, windowHeight) = getWindowSize();

    SDL_Rect viewport = getViewport();

    int slackWidth = 0;
    int slackHeight = 0;

    if (viewport.x) {
        auto logicalWidth = kVgaWidth * windowHeight / kVgaHeight;
        slackWidth = (windowWidth - logicalWidth) / 2;
    } else if (viewport.y) {
        auto logicalHeight = kVgaHeight * windowWidth / kVgaWidth;
        slackHeight = (windowHeight - logicalHeight) / 2;
    }

    if (m_y < slackHeight || m_y >= windowHeight - slackHeight)
        return false;

    if (m_x < slackWidth || m_x >= windowWidth - slackWidth)
        return false;

    m_x = (m_x - slackWidth) * kVgaWidth / (windowWidth - 2 * slackWidth);
    m_y = (m_y - slackHeight) * kVgaHeight / (windowHeight - 2 * slackHeight);

    return true;
}

static bool globalWheelHandler(int scrollAmount)
{
    assert(!m_mouseWheelData.empty());

    const auto& wheelData = m_mouseWheelData.back();
    bool handled = false;

    if (scrollAmount > 0 && wheelData.upEntry >= 0) {
        selectEntry(wheelData.upEntry);
        handled = true;
    } else if (scrollAmount < 0 && wheelData.downEntry >= 0) {
        selectEntry(wheelData.downEntry);
        handled = true;
    }

    return handled;
}

static void handleScrollerEntry(MenuEntry& entry)
{
    if (entry.onSelect && isEntryScroller(entry)) {
        m_scrolling = true;
        m_scrollEntry = &entry;
        m_lastScrollTick = SDL_GetTicks() + kScrollRateMs;
        selectEntry(m_scrollEntry);
    }
}

static bool performMouseWheelAction(const MenuEntry& entry, int scrollValue)
{
    assert(!m_mouseWheelData.empty());

    for (const auto& mouseWheelEntry : m_mouseWheelData.back().entries) {
        if (entry.ordinal >= mouseWheelEntry.first && entry.ordinal <= mouseWheelEntry.last) {
            int scrollUpEntryIndex = mouseWheelEntry.scrollUpEntry;
            int scrollDownEntryIndex = mouseWheelEntry.scrollDownEntry;

            assert(scrollUpEntryIndex >= 0 && scrollUpEntryIndex < 256);
            assert(scrollDownEntryIndex >= 0 && scrollDownEntryIndex < 256);

            auto scrollEntry = getMenuEntry(scrollValue > 0 ? scrollUpEntryIndex : scrollDownEntryIndex);
            if (scrollEntry->selectable()) {
                assert(!scrollEntry->onSelect.empty());
                scrollEntry->onSelect();
                return true;
            }

            break;
        }
    }

    return false;
}

static bool isPointInsideEntry(int x, int y, const MenuEntry& entry, int slack = 0)
{
    return x + slack >= entry.x && x - slack < entry.x + entry.width &&
        y + slack >= entry.y && y - slack < entry.y + entry.height;
}

static bool isPointerInsideEntry(const MenuEntry& entry, int slack = 0)
{
    return isPointInsideEntry(m_x, m_y, entry, slack);
}

template<typename F>
static MenuEntry *findPointedMenuEntry(F f, int slack = 0)
{
    auto currentMenu = getCurrentMenu();
    auto entries = currentMenu->entries();

    // make sure to check all the entries since there could be invisible/unreachable ones overlapping with selectable
    for (size_t i = 0; i < currentMenu->numEntries; i++) {
        auto& entry = entries[i];
        if (isPointerInsideEntry(entry, slack) && entry.selectable() && m_reachableEntries[entry.ordinal] && f(entry))
            return &entry;
    }

    return nullptr;
}

static MenuEntry *findPointedMenuEntry()
{
    return findPointedMenuEntry([](const auto&) { return true; });
}

// Allow certain area around the mouse pointer to trigger entries when mouse wheel scrolling
static void checkForExpandedWheelScrolling(int scrollAmount)
{
    constexpr int kMouseScrollSlack = 2;
    findPointedMenuEntry([scrollAmount](const auto& entry) { return performMouseWheelAction(entry, scrollAmount); }, kMouseScrollSlack);
}

static bool mouseMovedFromPreviousFrame()
{
    static int m_lastX, m_lastY;

    bool moved = m_lastX != m_x || m_lastY != m_y;

    m_lastX = m_x;
    m_lastY = m_y;

    return moved;
}

static void saveClickCoordinates(bool leftButtonJustPressed)
{
    if (leftButtonJustPressed) {
        m_clickX = m_x;
        m_clickY = m_y;
    }
}

static void checkForEntryClicksAndMouseWheelMovement(bool leftButtonJustReleased)
{
    auto currentMenu = getCurrentMenu();

    auto wheelScrollAmount = mouseWheelAmount();
    auto wheelHandled = globalWheelHandler(wheelScrollAmount);

    auto entry = m_currentEntryUnderPointer;

    if (!entry || !entry->selectable() || !isPointerInsideEntry(*entry))
        entry = findPointedMenuEntry();

    if (entry) {
        if (!(m_buttons & SDL_BUTTON_LMASK) && mouseMovedFromPreviousFrame())
            currentMenu->selectedEntry = entry;

        if (leftButtonJustReleased) {
logInfo("PRCKUSHKASH");
            if (isPointInsideEntry(m_clickX, m_clickY, *entry)) {
                currentMenu->selectedEntry = entry;
                triggerMenuFire();
            }
        } else if (m_buttons & SDL_BUTTON_LMASK) {
            handleScrollerEntry(*entry);
        } else if (!wheelHandled && wheelScrollAmount) {
            performMouseWheelAction(*entry, wheelScrollAmount);
        }

        // check again, in case handler hid/disabled it
        if (!entry->selectable())
            entry = nullptr;
    } else if (!wheelHandled && wheelScrollAmount) {
        checkForExpandedWheelScrolling(wheelScrollAmount);
    }

    assert(!entry || entry->selectable());
    m_currentEntryUnderPointer = entry;
}

// Turn delta x and y into direction via tangent with 15 degrees slack.
static int getDirectionMask(int dx, int dy)
{
    assert(dx || dy);

    // 3 | 1
    // --+--
    // 2 | 0
    int quadrantIndex = 2 * (dx < 0) + (dy < 0);

    dx = std::abs(dx);
    dy = std::abs(dy);

    int angleComparison = dx * dx + dy * (dy - 4 * dx);

    // 0 = 0..15 degrees, 1 = 15..75 degrees, 2 = 75..90 degrees
    int angleIndex = dy < 2 * dx ? angleComparison <= 0 : 1 + (angleComparison >= 0);

    static const int kDirections[4][3] = {
        { kRightMask, kDownMask | kRightMask, kDownMask },
        { kRightMask, kUpMask | kRightMask, kUpMask },
        { kLeftMask, kLeftMask | kDownMask, kDownMask },
        { kLeftMask, kUpMask | kLeftMask, kUpMask },
    };

    assert(quadrantIndex >= 0 && quadrantIndex < 4 && angleIndex >= 0 && angleIndex < 3);

    return kDirections[quadrantIndex][angleIndex];
}

static void checkForMouseDragging(bool leftButtonJustPressed)
{
    if (leftButtonJustPressed || !(m_buttons & SDL_BUTTON_LMASK))
        return;

    constexpr int kMinDragDistance = 4;

    auto selectedEntry = getCurrentMenu()->selectedEntry;

    if (selectedEntry && selectedEntry->controlMask & ~(kShortFireMask | kFireMask)) {
        assert(selectedEntry->selectable());

        int dx = m_x - m_clickX;
        int dy = m_y - m_clickY;

        if (std::abs(dx) > kMinDragDistance || std::abs(dy) > kMinDragDistance) {
            auto controlMask = getDirectionMask(dx, dy) | kFireMask;
logInfo("NANGHAAAYUUU");
            selectEntry(selectedEntry, controlMask);
        }
    }
}

void resetMenuMouseData()
{
    determineReachableEntries();
    m_currentEntryUnderPointer = nullptr;
    m_clickX = -1;
    m_clickY = -1;
}

void menuMouseOnAboutToShowNewMenu()
{
    m_mouseWheelData.push_back({});
}

void menuMouseOnOldMenuRestored()
{
    m_mouseWheelData.pop_back();
}

void setMouseWheelEntries(const MouseWheelEntryGroupList& mouseWheelEntries)
{
    assert(!m_mouseWheelData.empty());
    auto& wheelData = m_mouseWheelData.back();

    assert(!wheelData.globalHandlerActive());

    wheelData.entries = mouseWheelEntries;
    wheelData.resetGlobalHandler();
}

void setGlobalWheelEntries(int upEntry /* = -1 */, int downEntry /* = -1 */)
{
    assert(!m_mouseWheelData.empty());
    auto& wheelData = m_mouseWheelData.back();

    assert(wheelData.entries.empty());

    wheelData.upEntry = upEntry;
    wheelData.downEntry = downEntry;
    wheelData.entries.clear();
}

void updateMouse()
{
    bool leftButtonJustPressed, leftButtonJustReleased, rightButtonJustReleased;
    std::tie(leftButtonJustPressed, leftButtonJustReleased, rightButtonJustReleased) = updateMouseStatus();

    if (handleClickScrolling() || handleRightClickExitMenu(rightButtonJustReleased))
        return;

    if (hasMouseFocus() && !isMatchRunning() && mapCoordinatesToGameArea()) {
        saveClickCoordinates(leftButtonJustPressed);
        checkForEntryClicksAndMouseWheelMovement(leftButtonJustReleased);
        checkForMouseDragging(leftButtonJustPressed);
    }
}

// Finds and marks every reachable item starting from the initial one. We must track reachability in order
// to avoid selecting items with the mouse that are never supposed to be selected, like say labels.
void determineReachableEntries()
{
    static const void *s_lastMenu;

    if (s_lastMenu == getCurrentPackedMenu())
        return;

    s_lastMenu = getCurrentPackedMenu();

    std::fill(m_reachableEntries.begin(), m_reachableEntries.end(), false);

    static_assert(offsetof(MenuEntry, downEntry) == offsetof(MenuEntry, upEntry) + 1 &&
        offsetof(MenuEntry, upEntry) == offsetof(MenuEntry, rightEntry) + 1 &&
        offsetof(MenuEntry, rightEntry) == offsetof(MenuEntry, leftEntry) + 1 &&
        sizeof(MenuEntry::leftEntry) == 1, "MenuEntry: assumptions about next entries failed");

    std::vector<std::pair<const MenuEntry *, int>> entryStack;
    entryStack.reserve(256);

    auto currentMenu = getCurrentMenu();
    auto currentEntry = currentMenu->selectedEntry;
    auto entries = currentMenu->entries();

    entryStack.emplace_back(currentEntry, 0);

    while (!entryStack.empty()) {
        const MenuEntry *entry;
        int direction;

        std::tie(entry, direction) = entryStack.back();

        assert(entry && direction >= MenuEntry::kInitialDirection && direction <= MenuEntry::kNumDirections);
        assert(entry->ordinal < 256);

        m_reachableEntries[entry->ordinal] = true;

        auto nextEntries = reinterpret_cast<const uint8_t *>(&entry->leftEntry);

        for (; direction < MenuEntry::kNumDirections; direction++) {
            auto ord = nextEntries[direction];

            if (ord != 255 && !m_reachableEntries[ord]) {
                entryStack.emplace_back(&entries[ord], 0);
                break;
            }
        }

        if (direction >= MenuEntry::kNumDirections) {
            entryStack.pop_back();
        } else {
            assert(entryStack.size() >= 2);
            entryStack[entryStack.size() - 2].second = direction + 1;
        }
    }
}

bool isEntryReachable(int index)
{
    return m_reachableEntries[index];
}
