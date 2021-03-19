#pragma once

enum PlayerNumber {
    kNoPlayer = -1,
    kPlayer1 = 0,
    kPlayer2 = 1,
};

enum Controls {
    kNone,
    kKeyboard1,
    kKeyboard2,
    kJoypad,
    kMouse,                 // :D
    kNumControls,
    kNumUniqueControls = kNumControls - 1,
};

constexpr int kNoJoypad = INT_MIN;

Controls getPl1Controls();
Controls getPl2Controls();
bool setControls(PlayerNumber player, Controls controls, int joypadIndex = kNoJoypad);
void setPl1Controls(Controls controls, int joypadIndex = kNoJoypad);
void setPl2Controls(Controls controls, int joypadIndex = kNoJoypad);

bool anyInputActive();
bool mouseClickAndRelease();
void waitForKeyboardAndMouseIdle();
int mouseWheelAmount();

void waitForKeyPress();
std::pair<bool, SDL_Scancode> getKeyInterruptible();
void processControlEvents();

void setGlobalShortcutsEnabled(bool enabled);

void loadControlOptions(const CSimpleIni& ini);
void saveControlOptions(CSimpleIni& ini);

void initGameControls();

bool gotMousePlayer();
bool testForPlayerKeys();
