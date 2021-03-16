#pragma once

#include "gameControlEvents.h"
#include "controls.h"

enum class QuickConfigControls
{
    kKeyboard,
    kJoypad,
};

enum class QuickConfigStatus
{
    kNoInput,
    kContinue,
    kAbort,
    kDone,
    kAlreadyUsed,
    kTakenByOtherPlayer,
};

struct QuickConfigContext
{
    using GetControlFunction = std::pair<QuickConfigStatus, const char *> (*)(QuickConfigContext& context);
    using ResetFunction = std::function<void(QuickConfigContext& context)>;

    static constexpr int kElementNameBufferSize = 32;

    // VS is driving me crazy... it wouldn't compile without a constructor under x86 and C++17 only
    QuickConfigContext(PlayerNumber player, QuickConfigControls controls, int joypadIndex, const char *selectWhat, const char *abortText,
        GetControlFunction getControlFn, ResetFunction resetFn)
        : player(player), controls(controls), joypadIndex(joypadIndex), elementNames{}, selectWhat(selectWhat),
        abortText(abortText), getControlFn(getControlFn), resetFn(resetFn) {}

    PlayerNumber player;
    QuickConfigControls controls;
    int joypadIndex;
    char elementNames[kNumDefaultGameControlEvents][kElementNameBufferSize];
    char scratchBuf[kElementNameBufferSize];
    const char *selectWhat;
    const char *abortText;
    GetControlFunction getControlFn;
    ResetFunction resetFn;
    int currentSlot = -1;
    int warningY = -1;

    void reset() {
        currentSlot = -1;
        std::for_each(std::begin(elementNames), std::end(elementNames), [](auto *name) { name[0] = '\0'; });

        assert(resetFn);
        resetFn(*this);
    }

    std::pair<QuickConfigStatus, const char *> getNextControl() {
        assert(getControlFn);
        return getControlFn(*this);
    }

    static QuickConfigContext getKeyboardContext(PlayerNumber player, GetControlFunction get, ResetFunction reset) {
        return QuickConfigContext(player, QuickConfigControls::kKeyboard , kNoJoypad, "KEY", "(MOUSE CLICK ABORTS)", get, reset);
    }
    static QuickConfigContext getJoypadContext(PlayerNumber player, int joypadIndex, GetControlFunction get, ResetFunction reset) {
        return QuickConfigContext(player, QuickConfigControls::kJoypad, joypadIndex, "CONTROL", "(MOUSE CLICK/ESCAPE ABORTS)", get, reset);
    }
};

bool promptForDefaultGameEvents(QuickConfigContext& context);
QuickConfigStatus getNextControl(QuickConfigContext& context);
