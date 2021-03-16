#include "controls.h"
#include "gameControlEvents.h"
#include "keyBuffer.h"
#include "keyboard.h"
#include "joypads.h"
#include "mouse.h"
#include "render.h"
#include "options.h"
#include "game.h"
#include "util.h"

static Controls m_pl1Controls = kNone;
static Controls m_pl2Controls = kNone;

static int m_mouseWheelAmount;

static bool m_shortcutsEnabled = true;

// options
static constexpr char kControlsSection[] = "controls";

const char kPlayer1ControlsKey[] = "player1Controls";
const char kPlayer2ControlsKey[] = "player2Controls";

static const char *controlsToString(Controls controls)
{
    switch (controls) {
    case kNone: return "<none>";
    case kKeyboard1: return "keyboard 1";
    case kKeyboard2: return "keyboard 2";
    case kMouse: return "mouse";
    case kJoypad: return "game controller";
    default:
        assert(false);
        return "<unknown>";
    }
    static_assert(kNumControls == 5, "A control type is missing a string description");
}

Controls getPl1Controls()
{
    return m_pl1Controls;
}

Controls getPl2Controls()
{
    return m_pl2Controls;
}

void setControls(PlayerNumber player, Controls controls, int joypadIndex)
{
    assert(player == kPlayer1 || player == kPlayer2);

    auto& plControls = player == kPlayer1 ? m_pl1Controls : m_pl2Controls;
    auto otherControls = player == kPlayer1 ? m_pl2Controls : m_pl1Controls;
    auto otherPlayer = static_cast<PlayerNumber>(player ^ 1);
    auto currentJoypadIndex = player == kPlayer1 ? getPl1JoypadIndex() : getPl2JoypadIndex();

    if (controls != plControls || controls == kJoypad && joypadIndex != currentJoypadIndex) {
        logInfo("Setting player %d controls to %s", player == kPlayer1 ? 1 : 2, controlsToString(controls));

        plControls = controls;
        selectJoypadControls(player, joypadIndex);

        if (controls == kMouse && otherControls == kMouse) {
            auto otherKeyboard = player == kPlayer1 ? kKeyboard2 : kKeyboard1;
            setControls(otherPlayer, otherKeyboard);
        }
    }
}

void setPl1Controls(Controls controls, int joypadIndex /* = kNoJoypad */)
{
    setControls(kPlayer1, controls, joypadIndex);
}

void setPl2Controls(Controls controls, int joypadIndex /* = kNoJoypad */)
{
    setControls(kPlayer2, controls, joypadIndex);
}

static void checkQuitEvent()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (event.type == SDL_QUIT)
            std::exit(EXIT_SUCCESS);
}

static bool keyboardOrMouseActive()
{
    SDL_PumpEvents();

    int numKeys;
    auto keyState = SDL_GetKeyboardState(&numKeys);

    if (std::find(keyState, keyState + numKeys, 1) != keyState + numKeys)
        return true;

    if (SDL_GetMouseState(nullptr, nullptr))
        return true;

    return false;
}

bool anyInputActive()
{
    if (keyboardOrMouseActive())
        return true;

    if (getJoypadWithButtonDown() >= 0)
        return true;

    checkQuitEvent();

    return false;
}

bool mouseClickAndRelease()
{
    if (SDL_GetMouseState(nullptr, nullptr)) {
        do {
            SDL_PumpEvents();
            SDL_Delay(50);
        } while (SDL_GetMouseState(nullptr, nullptr));

        return true;
    }

    return false;
}

void waitForKeyboardAndMouseIdle()
{
    while (keyboardOrMouseActive())
        SDL_Delay(20);
}

int mouseWheelAmount()
{
    // for scrolling menu lists
    return m_mouseWheelAmount;
}

static void processEvent(const SDL_Event& event)
{
    switch (event.type) {
    case SDL_QUIT:
        std::exit(EXIT_SUCCESS);

    case SDL_KEYDOWN:
    case SDL_KEYUP:
        {
            auto key = event.key.keysym.scancode;
            bool pressed = event.type == SDL_KEYDOWN;

            if (m_shortcutsEnabled)
                checkKeyboardShortcuts(key, pressed);

            if (pressed)
                registerKey(key);
        }
        break;

    case SDL_JOYDEVICEADDED:
        logInfo("Adding joypad %d", event.jdevice.which);
        addNewJoypad(event.jdevice.which);
        break;

    case SDL_JOYDEVICEREMOVED:
        logInfo("Removing joypad %d", event.jdevice.which);
        removeJoypad(event.jdevice.which);
        break;

    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
        updateMouseButtons(event.button.button, event.button.state);
        break;

    case SDL_MOUSEMOTION:
        updateMouseMovement(event.motion.xrel, event.motion.yrel, event.motion.state);
        break;

    case SDL_MOUSEWHEEL:
        if (event.wheel.y)
            m_mouseWheelAmount = event.wheel.y;
        break;
    }
}

void waitForKeyPress()
{
    SDL_Event event;

    while (true) {
        if (SDL_WaitEvent(&event)) {
            processEvent(event);

            if (event.type == SDL_KEYDOWN)
                return;
        }
    }
}

std::pair<bool, SDL_Scancode> getKeyInterruptible()
{
    if (mouseClickAndRelease())
        return { true, SDL_SCANCODE_UNKNOWN };

    processControlEvents();
    auto key = getKey();

    if (numKeysInBuffer() > 3)
        flushKeyBuffer();

    return { false, key };
}

void processControlEvents()
{
    m_mouseWheelAmount = 0;

    SDL_Event event;

    while (SDL_PollEvent(&event))
        processEvent(event);

    if (swos.paused)
        SDL_Delay(15);
}

void setGlobalShortcutsEnabled(bool enabled)
{
    m_shortcutsEnabled = enabled;
}

void loadControlOptions(const CSimpleIni& ini)
{
    loadJoypadOptions(kControlsSection, ini);
    loadKeyboardConfig(ini);

    m_pl1Controls = kKeyboard1;
    auto pl1Controls = ini.GetLongValue(kControlsSection, kPlayer1ControlsKey);
    if (pl1Controls >= 0 && pl1Controls < kNumControls)
        m_pl1Controls = static_cast<Controls>(pl1Controls);

    m_pl2Controls = kNone;
    auto pl2Controls = ini.GetLongValue(kControlsSection, kPlayer2ControlsKey);
    if (pl2Controls >= 0 && pl2Controls < kNumControls)
        m_pl2Controls = static_cast<Controls>(pl2Controls);

    logInfo("Controls set to: %s (player 1), %s (player 2)", controlsToString(m_pl1Controls), controlsToString(m_pl2Controls));
}

void printEventInfoComment(CSimpleIni& ini)
{
    char comment[512] = "; ";
    auto sentinel = comment + sizeof(comment);
    auto p = comment + 2;

    traverseEvents(kGameEventAll, [&](auto event) {
        p += gameControlEventToString(event, p, sentinel - p);
        if (p < sentinel)
            p += snprintf(p, sentinel - p, " = 0x%x\n; ", event);
    });

    assert(p > comment + 4 && p < sentinel && p[-2] == ';');

    p[-2] = '\0';

    for (auto q = comment; *q; q++)
        *q = tolower(*q);

    ini.SetValue(kControlsSection, nullptr, nullptr, comment);
}

void saveControlOptions(CSimpleIni& ini)
{
    printEventInfoComment(ini);

    const char kInputControlsComment[] = "; 0 = none, 1 = keyboard, 2 = mouse, 3 = game controller";

    ini.SetLongValue(kControlsSection, kPlayer1ControlsKey, m_pl1Controls, kInputControlsComment);
    ini.SetLongValue(kControlsSection, kPlayer2ControlsKey, m_pl2Controls);

    saveJoypadOptions(kControlsSection, ini);
    saveKeyboardConfig(ini);
}

void initGameControls()
{
    resetMouse();

    // TODO: kill with fire when possible
    swos.pl1Fire = 0;
    swos.pl1SecondaryFire = 0;
    swos.pl1ShortFire = 0;
    swos.pl1FireCounter = 0;
    swos.pl1Direction = -1;
    swos.pl2Fire = 0;
    swos.pl2SecondaryFire = 0;
    swos.pl2ShortFire = 0;
    swos.pl2FireCounter = 0;
    swos.pl2Direction = -1;
}

bool gotMousePlayer()
{
    return m_pl1Controls == kMouse || m_pl2Controls == kMouse;
}

// Returns true if last pressed key belongs to selected keys of any currently active player.
bool testForPlayerKeys()
{
    auto key = peekKey();

    return m_pl1Controls == kKeyboard1 && pl1HasScancode(key) ||
        m_pl2Controls == kKeyboard2 && pl2HasScancode(key);
}

// outputs:
//   ASCII code in convertedKey
//   scan code in lastKey
//   0 if nothing pressed
//
void SWOS::GetKey()
{
    processControlEvents();

    swos.convertedKey = 0;
    swos.lastKey = 0;

    auto lastKey = getKey();
    if (lastKey != SDL_SCANCODE_UNKNOWN) {
        auto pcScanCode = sdlScancodeToPc(lastKey);
        if (pcScanCode != 255) {
            swos.lastKey = sdlScancodeToPc(lastKey);
            swos.convertedKey = swos.convertKeysTable[swos.lastKey];
        }
    }
}

__declspec(naked) void SWOS::JoyKeyOrCtrlPressed()
{
#ifdef SWOS_VM
    auto result = anyInputActive();
    SwosVM::flags.zero = result;
#else
    __asm {
        push ecx
        call anyInputActive
        test eax, eax
        pop  ecx
        retn
    }
#endif
}
