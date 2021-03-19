#include "joypads.h"
#include "Joypad.h"
#include "VirtualJoypad.h"
#include "SdlMappingParser.h"
#include "JoypadConfigRegister.h"
#include "keyboard.h"
#include "util.h"

static int m_pl1Joypad = kNoJoypad;
static int m_pl2Joypad = kNoJoypad;

static bool m_joypadsInitialized;
static bool m_autoConnectJoypads = true;
static bool m_disableMenuControllers = false;
static bool m_showSelectMatchControlsMenu = true;

static std::vector<Joypad> m_joypads;
static std::vector<VirtualJoypad> m_virtualJoypads;

static JoypadConfigRegister m_joypadConfig;

static constexpr char kAutoConnectJoypadsKey[] = "autoConnectControllers";
static constexpr char kDisableMenuControllers[] = "disableGameControllersInMenu";
static constexpr char kShowSelectMatchControlsMenu[] = "showSelectMatchControlsMenu";

static constexpr char kJoypad1Key[] = "player1Controller";
static constexpr char kJoypad2Key[] = "player2Controller";

static std::string m_joy1GuidStr;
static std::string m_joy2GuidStr;

static bool virtualJoypadIndex(int& index)
{
    if (index < 0) {
        index = -index - 1;
        assert(index < static_cast<int>(m_virtualJoypads.size()));
        return true;
    }

    return false;
}

static int joypadIndexToVirtual(int index)
{
    return -(index + 1);
}

static bool isRealJoypadIndex(int index)
{
    return index >= 0;
}

static SDL_UNUSED inline bool isValidJoypadIndex(int index)
{
    return index < 0 ? (-index - 1) < static_cast<int>(m_virtualJoypads.size()) : index < static_cast<int>(m_joypads.size());
}

static bool bothPlayersUsingJoypads()
{
    return getPl1Controls() == kJoypad && getPl2Controls() == kJoypad;
}

static int findFreeJoypad()
{
    for (int i = static_cast<int>(m_joypads.size()) - 1; i >= 0; i--)
        if (i != m_pl1Joypad && i != m_pl2Joypad)
            return i;

    for (int i = static_cast<int>(m_virtualJoypads.size()) - 1; i >= 0; i--)
        if (i != m_pl1Joypad && i != m_pl2Joypad)
            return joypadIndexToVirtual(i);

    return kNoJoypad;
}

static GameControlEvents joypadEvents(PlayerNumber player)
{
    int index = player == kPlayer1 ? m_pl1Joypad : m_pl2Joypad;
    return index == kNoJoypad ? kNoGameEvents : joypadEvents(index);
}

GameControlEvents pl1JoypadEvents()
{
    return joypadEvents(kPlayer1);
}

GameControlEvents pl2JoypadEvents()
{
    return joypadEvents(kPlayer2);
}

template<typename C>
GameControlEvents eventsFromAllJoypads(const C& c)
{
    auto events = kNoGameEvents;

    for (const auto& joypad : c) {
        events |= joypad.events();
        if (joypad.anyUnassignedButtonDown())
            events |= kGameEventKick;
    }

    return events;
}

GameControlEvents eventsFromAllJoypads()
{
    auto events = eventsFromAllJoypads(m_joypads);
    events |= eventsFromAllJoypads(m_virtualJoypads);
    return events;
}

GameControlEvents joypadEvents(int index)
{
    assert(isValidJoypadIndex(index));

    if (virtualJoypadIndex(index))
        return m_virtualJoypads[index].events();
    else
        return m_joypads[index].events();
}

JoypadElementValueList joypadElementValues(int index)
{
    assert(isValidJoypadIndex(index));

    if (virtualJoypadIndex(index))
        return m_virtualJoypads[index].elementValues();
    else
        return m_joypads[index].elementValues();
}

bool joypadHasBasicBindings(int index)
{
    assert(isValidJoypadIndex(index));

    GameControlEvents allEvents;

    if (virtualJoypadIndex(index))
        allEvents = m_virtualJoypads[index].allEventsMask();
    else
        allEvents = m_joypads[index].allEventsMask();

    return (allEvents & kMinimumGameEventsMask) == kMinimumGameEventsMask;
}

static std::pair<int, int> findJoypadsByGuids(const SDL_JoystickGUID& guid1, const SDL_JoystickGUID& guid2)
{
    auto result = std::make_pair(kNoJoypad, kNoJoypad);

    auto foundBoth = [&result]() { return result.first != kNoJoypad && result.second != kNoJoypad; };
    auto checkPl1 = [&result]() { return getPl1Controls() == kJoypad && result.first == kNoJoypad; };
    auto checkPl2 = [&result]() { return getPl2Controls() == kJoypad && result.second == kNoJoypad; };

    for (size_t i = 0; i < m_joypads.size() && !foundBoth(); i++) {
        if (checkPl1() && m_joypads[i].isOpen() && m_joypads[i].guidEqual(guid1))
            result.first = i;
        else if (checkPl2() && m_joypads[i].isOpen() && m_joypads[i].guidEqual(guid2))
            result.second = i;
    }

    if (!foundBoth()) {
        for (size_t i = 0; i < m_virtualJoypads.size() && !foundBoth(); i++) {
            if (checkPl1() && m_virtualJoypads[i].guidEqual(guid1))
                result.first = joypadIndexToVirtual(i);
            else if (checkPl2() && m_virtualJoypads[i].guidEqual(guid2))
                result.second = joypadIndexToVirtual(i);
        }
    }

    return result;
}

enum SiblingJoypadConfig {
    kNoMatch,
    kPrimary,
    kSecondary,
};

// Checks the status of other sibling (same GUID -- model) joypad.
static SiblingJoypadConfig siblingJoypadConfig(const Joypad& joypad, int index)
{
    if (bothPlayersUsingJoypads()) {
        int joy1Index = getPl1JoypadIndex();
        int joy2Index = getPl2JoypadIndex();
        int otherIndex = joy1Index == index ? joy2Index : joy1Index;

        // in case the indices haven't been set yet at the start
        if (otherIndex == kNoJoypad)
            return kNoMatch;

        if (guidEqual(joypad.guid(), joypadGuid(otherIndex))) {
            const auto otherConfig = joypadConfig(joy1Index);
            assert(otherConfig);
            return otherConfig->primary() ? kPrimary : kSecondary;
        }
    }

    return kNoMatch;
}

static void handleTwoSameGuidJoypadsSelected(Joypad& joypad, int joypadIndex)
{
    auto config = joypadConfig(joypadIndex);
    assert(config);

    auto status = siblingJoypadConfig(joypad, joypadIndex);
    if (status == kPrimary && config->primary() || status == kSecondary && !config->primary()) {
        bool switchToSecondary = config->primary();
        logInfo("Switching joypad config to %s", switchToSecondary ? "secondary" : "primary");
        config = m_joypadConfig.config(joypad.guid(), switchToSecondary);
        joypad.setConfig(config);
    }
}

static void updateJoypadLastSelected(int index)
{
    auto now = getMillisecondsSinceEpoch();
    if (virtualJoypadIndex(index))
        m_virtualJoypads[index].updateLastSelected(now);
    else
        m_joypads[index].updateLastSelected(now);
}

static void setPl1JoypadIndex(int index)
{
    assert(index == kNoJoypad || isValidJoypadIndex(index));

    m_pl1Joypad = index;
    if (index != kNoJoypad) {
        logInfo("Player 1 using joypad '%s'", joypadName(index));
        updateJoypadLastSelected(index);
    }
}

static void setPl2JoypadIndex(int index)
{
    assert(index == kNoJoypad || isValidJoypadIndex(index));

    m_pl2Joypad = index;
    if (index != kNoJoypad) {
        logInfo("Player 2 using joypad '%s'", joypadName(index));
        updateJoypadLastSelected(index);
    }
}

// Tries to assign some of the currently connected joypads to users based on configuration from the ini file.
// Called at the start of the program, after SDL joysticks have been initialized and the joypad config was loaded from the ini file.
void assignJoypadsToPlayers()
{
    if (getPl1Controls() != kJoypad && getPl2Controls() != kJoypad)
        return;

    processControlEvents();

    auto joy1Guid = SDL_JoystickGetGUIDFromString(m_joy1GuidStr.c_str());
    auto joy2Guid = SDL_JoystickGetGUIDFromString(m_joy2GuidStr.c_str());

    auto indices = findJoypadsByGuids(joy1Guid, joy2Guid);

    if (indices.first != kNoJoypad) {
        logInfo("Found joypad for player 1 with GUID %s", m_joy1GuidStr.c_str());
        setPl1JoypadIndex(indices.first);
    }
    if (indices.second != kNoJoypad) {
        logInfo("Found joypad for player 2 with GUID %s", m_joy2GuidStr.c_str());
        setPl2JoypadIndex(indices.second);
    }

    // in case we found nothing by GUID's, just assign any
    if (getPl1Controls() == kJoypad && m_pl1Joypad == kNoJoypad) {
        int index = findFreeJoypad();
        if (index != kNoJoypad)
            setPl1JoypadIndex(index);
    }
    if (getPl2Controls() == kJoypad && m_pl2Joypad == kNoJoypad) {
        int index = findFreeJoypad();
        if (index != kNoJoypad)
            setPl2JoypadIndex(index);
    }

    // revert controls if no joypads after all
    if (getPl1Controls() == kJoypad && m_pl1Joypad == kNoJoypad)
        setPl1Controls(pl1HasBasicBindings() ? kKeyboard1 : kMouse);

    if (getPl2Controls() == kJoypad && m_pl2Joypad == kNoJoypad)
        setPl2Controls(kNone);

    if (bothPlayersUsingJoypads())
        handleTwoSameGuidJoypadsSelected(m_joypads[m_pl2Joypad], m_pl2Joypad);

    assert(getPl1Controls() != kJoypad || isValidJoypadIndex(m_pl1Joypad) && joypadId(m_pl1Joypad) != Joypad::kInvalidJoypadId);
    assert(getPl2Controls() != kJoypad || isValidJoypadIndex(m_pl2Joypad) && joypadId(m_pl2Joypad) != Joypad::kInvalidJoypadId);

    m_joypadsInitialized = true;

    m_joy1GuidStr.clear();
    m_joy2GuidStr.clear();
}

int getPl1JoypadIndex()
{
    return m_pl1Joypad;
}

int getPl2JoypadIndex()
{
    return m_pl2Joypad;
}

static bool firstSelectedJoypad(int index1, int index2)
{
    auto lastSelected = [](int index) {
        return virtualJoypadIndex(index) ? m_virtualJoypads[index].lastSelected() : m_joypads[index].lastSelected();
    };

    return lastSelected(index1) < lastSelected(index2);
}

int getNumJoypads()
{
    return m_joypads.size() + m_virtualJoypads.size();
}

template<typename C, typename F>
static int findJoypad(const C& c, F f)
{
    auto it = std::find_if(c.begin(), c.end(), f);

    if (it != c.end())
        return it - c.begin();

    return kNoJoypad;
}

template<typename C>
static int findJoypadId(const C& c, SDL_JoystickID id)
{
    return findJoypad(c, [id](const auto& joypad) { return joypad.id() == id; });
}

static int getJoypadIndexFromId(SDL_JoystickID id)
{
    return id < 0 ? findJoypadId(m_virtualJoypads, id) : findJoypadId(m_joypads, id);
}

bool joypadDisconnected(SDL_JoystickID id)
{
    int index = getJoypadIndexFromId(id);
    return index == kNoJoypad;
}

JoypadConfig *joypadConfig(int index)
{
    assert(isValidJoypadIndex(index));

    if (virtualJoypadIndex(index))
        return m_virtualJoypads[index].config();
    else if (index < static_cast<int>(m_joypads.size()))
        return m_joypads[index].config();

    return nullptr;
}

const char *joypadName(int index)
{
    assert(isValidJoypadIndex(index));

    if (virtualJoypadIndex(index))
        return m_virtualJoypads[index].name();
    else if (index < static_cast<int>(m_joypads.size()))
        return m_joypads[index].name();

    return nullptr;
}

SDL_JoystickGUID joypadGuid(int index)
{
    assert(isValidJoypadIndex(index));

    if (virtualJoypadIndex(index))
        return m_virtualJoypads[index].guid();
    else if (index < static_cast<int>(m_joypads.size()))
        return m_joypads[index].guid();

    return {};
}

SDL_JoystickID joypadId(int index)
{
    assert(isValidJoypadIndex(index));

    if (index >= 0 && index < static_cast<int>(m_joypads.size()))
        return m_joypads[index].id();

    return -1;
}

const char *joypadPowerLevel(int index)
{
    assert(isValidJoypadIndex(index));

    if (virtualJoypadIndex(index))
        return "OVER 9000";
    else
        return m_joypads[index].powerLevel();
}

int joypadNumHats(int index)
{
    assert(isValidJoypadIndex(index));
    return virtualJoypadIndex(index) ? m_virtualJoypads[index].numHats() : m_joypads[index].numHats();
}

int joypadNumButtons(int index)
{
    assert(isValidJoypadIndex(index));
    return virtualJoypadIndex(index) ? m_virtualJoypads[index].numButtons() : m_joypads[index].numButtons();
}

int joypadNumAxes(int index)
{
    assert(isValidJoypadIndex(index));
    return virtualJoypadIndex(index) ? m_virtualJoypads[index].numAxes() : m_joypads[index].numAxes();
}

int joypadNumBalls(int index)
{
    assert(isValidJoypadIndex(index));
    return virtualJoypadIndex(index) ? m_virtualJoypads[index].numBalls() : m_joypads[index].numBalls();
}

JoypadConfig::HatBindingList *joypadHatBindings(int joypadIndex, int hatIndex)
{
    assert(isValidJoypadIndex(joypadIndex));

    auto config = joypadConfig(joypadIndex);
    return config->getHatBindings(hatIndex);
}

JoypadConfig::AxisIntervalList *joypadAxisIntervals(int joypadIndex, int axisIndex)
{
    assert(isValidJoypadIndex(joypadIndex));

    auto config = joypadConfig(joypadIndex);
    return config->getAxisIntervals(axisIndex);
}

JoypadConfig::BallBinding& joypadBall(int joypadIndex, int ballIndex)
{
    assert(isValidJoypadIndex(joypadIndex));

    auto config = joypadConfig(joypadIndex);
    return config->getBall(ballIndex);
}

bool tryReopeningJoypad(int index)
{
    return virtualJoypadIndex(index) ? true : m_joypads[index].tryReopening(index);
}

static void autoSelectJoypad(int index)
{
    assert(isRealJoypadIndex(index) && m_joypads[index].isOpen());

    if (getPl2Controls() != kJoypad) {
        setPl2Controls(kJoypad, index);
    } else if (getPl1Controls() != kJoypad) {
        setPl1Controls(kJoypad, index);
    } else {
        assert(isValidJoypadIndex(m_pl1Joypad) && isValidJoypadIndex(m_pl2Joypad));

        // kick out the one that was first to connect
        if (firstSelectedJoypad(m_pl1Joypad, m_pl2Joypad))
            setPl1Controls(kJoypad, index);
        else
            setPl2Controls(kJoypad, index);
    }
}

static void assignNewJoypadConfig(Joypad& joypad, int index)
{
    bool secondaryConfig = siblingJoypadConfig(joypad, index) == kPrimary;
    auto config = m_joypadConfig.config(joypad.guid(), secondaryConfig);
    joypad.setConfig(config);
}

static void assignInstanceNumber(Joypad& joypad)
{
    int numSameNameJoypads = 0;
    Joypad *firstJoypad{};

    for (auto& currentJoypad : m_joypads) {
        if (&joypad != &currentJoypad && joypad.hash() == currentJoypad.hash() && !strcmp(joypad.name(), currentJoypad.name())) {
            if (!firstJoypad)
                firstJoypad = &currentJoypad;
            numSameNameJoypads++;
        }
    }

    if (numSameNameJoypads > 0) {
        if (numSameNameJoypads == 1) {
            assert(firstJoypad);
            firstJoypad->setNameSuffix(1);
        }
        joypad.setNameSuffix(numSameNameJoypads + 1);
    }
}

void addNewJoypad(int index)
{
    assert(index <= SDL_NumJoysticks());

    m_joypads.emplace(m_joypads.begin() + index, index);
    auto& joypad = m_joypads[index];

    assignInstanceNumber(joypad);

    // index shouldn't ever be less than the number of joypads, but just in case...
    m_pl1Joypad += m_pl1Joypad >= index;
    m_pl2Joypad += m_pl2Joypad >= index;

    assignNewJoypadConfig(joypad, index);

    if (m_joypadsInitialized && joypad.isOpen() && m_autoConnectJoypads)
        autoSelectJoypad(index);
}

void removeJoypad(SDL_JoystickID id)
{
    auto it = std::find_if(m_joypads.begin(), m_joypads.end(), [id](const auto& joypad) { return joypad.id() == id; });

    if (it != m_joypads.end()) {
        int index = it - m_joypads.begin();

        if (index == m_pl1Joypad) {
            logInfo("Player 1 joypad disconnected");
            int pl1Joypad = findFreeJoypad();
            if (pl1Joypad == kNoJoypad) {
                logInfo("Can't find another joypad for player 1, switching to keyboard");
                setPl1Controls(kKeyboard1);
            } else {
                auto joyName = joypadName(pl1Joypad);
                logInfo("Player 1 switching to joypad %d (%s)", pl1Joypad, joyName);
                setPl1JoypadIndex(pl1Joypad);
            }
        } else if (index == m_pl2Joypad) {
            logInfo("Player 2 joypad disconnected");
            int pl2Joypad = findFreeJoypad();
            if (pl2Joypad == kNoJoypad) {
                logInfo("Can't find another joypad for player 2, switching to keyboard");
                setPl2Controls(kKeyboard2);
            } else {
                auto joyName = joypadName(pl2Joypad);
                logInfo("Player 2 switching to joypad %d (%s)", pl2Joypad, joyName);
                setPl2JoypadIndex(pl2Joypad);
            }
        }

        m_joypads.erase(it);

        // update all the indices
        m_pl1Joypad -= m_pl1Joypad > index;
        m_pl2Joypad -= m_pl2Joypad > index;
    } else {
        logWarn("Joypad id %d not found!", id);
        assert(false);
    }
}

template<typename C>
static int getJoypadWithButtonDown(const C& c)
{
    return findJoypad(c, [](const auto& joypad) { return joypad.anyButtonDown(); });
}

int getJoypadWithButtonDown()
{
    int index = getJoypadWithButtonDown(m_joypads);
    if (index != kNoJoypad)
        return index;

    return getJoypadWithButtonDown(m_virtualJoypads);
}

void waitForJoypadButtonsIdle()
{
    do {
        processControlEvents();
    } while (getJoypadWithButtonDown() >= 0);

    swos.fire = 0;
}

// Remove the control of the joypad from this player.
static void unassignJoypad(PlayerNumber player)
{
    assert((player == kPlayer1 ? getPl1Controls() : getPl2Controls()) == kJoypad);

    logInfo("Trying to remove joypad %d from player %d", player == kPlayer1 ? getPl1JoypadIndex() : getPl2JoypadIndex(), player + 1);

    int joypadIndex = findFreeJoypad();
    assert(joypadIndex == kNoJoypad || isValidJoypadIndex(joypadIndex));

    if (joypadIndex == kNoJoypad) {
        if (player == kPlayer1)
            setPl1Controls(pl1HasBasicBindings() ? kKeyboard1 : kMouse);
        else
            setPl2Controls(kNone);
    } else {
        if (player == kPlayer1)
            setPl1JoypadIndex(joypadIndex);
        else
            setPl2JoypadIndex(joypadIndex);
    }
}

bool setJoypad(PlayerNumber player, int joypadIndex)
{
    assert(player == kPlayer1 || player == kPlayer2);
    assert(joypadIndex == kNoJoypad || isValidJoypadIndex(joypadIndex));
    assert(joypadIndex == kNoJoypad || (player == kPlayer1 ? getPl1Controls() == kJoypad : getPl2Controls() == kJoypad));

    int playerNo = player == kPlayer1 ? 1 : 2;
    if (joypadIndex != kNoJoypad)
        logInfo("Selecting controller %d for player %d", joypadIndex, playerNo);
    else
        logInfo("Unsetting controller for player %d", playerNo);

    if (player == kPlayer1)
        setPl1JoypadIndex(joypadIndex);
    else
        setPl2JoypadIndex(joypadIndex);

    if (m_pl1Joypad == m_pl2Joypad && m_pl1Joypad != kNoJoypad) {
        auto otherPlayer = player == kPlayer1 ? kPlayer2 : kPlayer1;
        unassignJoypad(otherPlayer);
    }

    if (isRealJoypadIndex(joypadIndex)) {
        auto& joypad = m_joypads[joypadIndex];
        if (!joypad.tryReopening(joypadIndex)) {
            logWarn("Failed to open joypad %d", joypadIndex);
            beep();
            return false;
        }

        handleTwoSameGuidJoypadsSelected(joypad, joypadIndex);
    }

    return true;
}

bool getAutoConnectJoypads()
{
    return m_autoConnectJoypads;
}

void setAutoConnectJoypads(bool value)
{
    m_autoConnectJoypads = value;
}

bool getDisableMenuControllers()
{
    return m_disableMenuControllers;
}

void setDisableMenuControllers(bool value)
{
    m_disableMenuControllers = value;
}

bool getShowSelectMatchControlsMenu()
{
    return m_showSelectMatchControlsMenu;
}

void setShowSelectMatchControlsMenu(bool value)
{
    m_showSelectMatchControlsMenu = value;
}

//
// joypad options
//

void loadJoypadOptions(const char *controlsSection, const CSimpleIni& ini)
{
    m_joypadConfig.loadConfig(ini);

    auto joy1GuidStr = ini.GetValue(controlsSection, kJoypad1Key);
    m_joy1GuidStr = joy1GuidStr ? joy1GuidStr : "";

    auto joy2GuidStr = ini.GetValue(controlsSection, kJoypad2Key);
    m_joy2GuidStr = joy2GuidStr ? joy2GuidStr : "";

    m_autoConnectJoypads = ini.GetBoolValue(controlsSection, kAutoConnectJoypadsKey, true);
    m_disableMenuControllers = ini.GetBoolValue(controlsSection, kDisableMenuControllers, false);
    m_showSelectMatchControlsMenu = ini.GetBoolValue(controlsSection, kShowSelectMatchControlsMenu, true);
}

void saveJoypadOptions(const char *controlsSection, CSimpleIni& ini)
{
    const auto kJoypadSaveData = {
        std::make_tuple(getPl1Controls(), m_pl1Joypad, kJoypad1Key),
        std::make_tuple(getPl2Controls(), m_pl2Joypad, kJoypad2Key),
    };

    for (const auto& joypadData : kJoypadSaveData) {
        auto controls = std::get<0>(joypadData);
        auto joypadIndex = std::get<1>(joypadData);
        auto key = std::get<2>(joypadData);

        if (controls == kJoypad) {
            assert(joypadIndex != kNoJoypad);

            auto guid = joypadGuid(joypadIndex);
            char guidString[64];
            SDL_JoystickGetGUIDString(guid, guidString, sizeof(guidString));

            assert(std::count(guidString, guidString + 32, '0') != 32);

            ini.SetValue(controlsSection, key, guidString);
        }
    }

    ini.SetBoolValue(controlsSection, kAutoConnectJoypadsKey, m_autoConnectJoypads);
    ini.SetBoolValue(controlsSection, kDisableMenuControllers, m_disableMenuControllers);
    ini.SetBoolValue(controlsSection, kShowSelectMatchControlsMenu, m_showSelectMatchControlsMenu);

    m_joypadConfig.saveConfig(ini);
}
