#include "quickConfigMenu.h"
#include "menus.h"
#include "text.h"
#include "controls.h"
#include "scancodes.h"
#include "keyBuffer.h"
#include "joypads.h"

constexpr int kRedefineKeysHeaderY = 30;
constexpr int kRedefineKeysStartY = 70;
constexpr int kRedefineKeysPromptX = 160;
constexpr int kRedefineKeysColumn1X = 108;
constexpr int kFinalPromptY = 140;
constexpr int kWarningY = 172;

enum class FinalPromptResult {
    kSuccess,
    kFailure,
    kRestart,
};

static void drawQuickConfigMenu(const QuickConfigContext& context);
static FinalPromptResult waitForConfirmation();

bool promptForDefaultGameEvents(QuickConfigContext& context)
{
    assert(context.player == kPlayer1 || context.player == kPlayer2);
    assert(context.controls == QuickConfigControls::kKeyboard || context.controls == QuickConfigControls::kJoypad);

    context.reset();
    if (context.warningY < 0)
        context.warningY = kWarningY;

    logInfo("Configuring %s for player %d", context.controls == QuickConfigControls::kKeyboard ? "keyboard" : "joypad",
        context.player == kPlayer1 ? 1 : 2);
    if (context.controls == QuickConfigControls::kJoypad)
        logInfo("Joypad: #%d", context.joypadIndex);

    waitForKeyboardAndMouseIdle();

    while (true) {
        drawQuickConfigMenu(context);

        auto result = getNextControl(context);

        switch (result) {
        case QuickConfigStatus::kContinue:
            continue;
        case QuickConfigStatus::kAbort:
            return false;
        case QuickConfigStatus::kDone:
            break;
        default:
            assert(false);
        }

        drawQuickConfigMenu(context);

        switch (waitForConfirmation()) {
        case FinalPromptResult::kSuccess:
            return true;
        case FinalPromptResult::kFailure:
            return false;
        case FinalPromptResult::kRestart:
            context.reset();
            break;
        }
    }
}

static void clearWarningIfNeeded(Uint32& showWarningTicks, int warningY);
static Uint32 printWarning(QuickConfigStatus status, const char *control, const QuickConfigContext& context, char *warningBuffer, size_t warningBufferSize);

QuickConfigStatus getNextControl(QuickConfigContext& context)
{
    constexpr int kWarningBufferSize = 64;
    char warningBuffer[kWarningBufferSize];

    Uint32 showWarningTicks = 0;

    while (true) {
        processControlEvents();

        clearWarningIfNeeded(showWarningTicks, context.warningY);

        auto result = context.getNextControl();

        switch (result.first) {
        case QuickConfigStatus::kAbort:
        case QuickConfigStatus::kDone:
        case QuickConfigStatus::kContinue:
            return result.first;
        }

        if (result.first != QuickConfigStatus::kNoInput) {
            assert(result.second);
            showWarningTicks = printWarning(result.first, result.second, context, warningBuffer, kWarningBufferSize);
        }

        SDL_Delay(50);
    }
}

static void drawControls(const QuickConfigContext& context)
{
    constexpr int kRowGap = 10;
    constexpr int kControlX = kRedefineKeysColumn1X + 40;
    constexpr int kBlockSpriteX = kControlX - 2;

    int y = kRedefineKeysStartY;

    for (int i = 0; i < kNumDefaultGameControlEvents; i++) {
        if (context.elementNames[i][0]) {
            drawMenuText(kControlX, y, context.elementNames[i]);
            y += kRowGap;
        }
    }

    if (context.currentSlot < kNumDefaultGameControlEvents)
        drawMenuSprite(kBlockSpriteX, y, kBlockSpriteIndex);
}

static void drawQuickConfigMenu(const QuickConfigContext& context)
{
    using namespace SwosVM;

    assert(context.player == kPlayer1 || context.player == kPlayer2);

    char buf[128];
    snprintf(buf, sizeof(buf), "SELECT %sS FOR PLAYER %d", context.selectWhat, context.player == kPlayer1 ? 1 : 2);

    redrawMenuBackground();
    drawMenuTextCentered(kRedefineKeysPromptX, kRedefineKeysHeaderY, buf);

    drawMenuText(kRedefineKeysColumn1X, kRedefineKeysStartY, "UP:");
    drawMenuText(kRedefineKeysColumn1X, kRedefineKeysStartY + 10, "DOWN:");
    drawMenuText(kRedefineKeysColumn1X, kRedefineKeysStartY + 20, "LEFT:");
    drawMenuText(kRedefineKeysColumn1X, kRedefineKeysStartY + 30, "RIGHT:");
    drawMenuText(kRedefineKeysColumn1X, kRedefineKeysStartY + 40, "FIRE:");
    drawMenuText(kRedefineKeysColumn1X, kRedefineKeysStartY + 50, "BENCH:");

    constexpr int kAbortY = kRedefineKeysStartY + 50 + kRedefineKeysStartY - kRedefineKeysHeaderY;

    static_assert(kWarningY >= kAbortY + 10, "Warning and abort labels overlap");

    drawMenuTextCentered(160, kAbortY, context.abortText);

    drawControls(context);

    SWOS::FlipInMenu();
}

static FinalPromptResult waitForConfirmation()
{
    drawMenuTextCentered(kRedefineKeysPromptX, kFinalPromptY,
        "ENTER/S - SAVE, ESCAPE/D - DISCARD, R - RESTART");

    SWOS::FlipInMenu();

    do {
        processControlEvents();

        auto key = getKeyInterruptible();
        waitForKeyboardAndMouseIdle();

        if (key.first)
            return FinalPromptResult::kFailure;

        switch (key.second) {
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_RETURN2:
        case SDL_SCANCODE_KP_ENTER:
            return FinalPromptResult::kSuccess;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_ESCAPE:
            return FinalPromptResult::kFailure;
        case SDL_SCANCODE_R:
            return FinalPromptResult::kRestart;
        }

        SDL_Delay(50);
    } while (true);
}

static void clearWarningIfNeeded(Uint32& showWarningTicks, int warningY)
{
    if (showWarningTicks && showWarningTicks < SDL_GetTicks()) {
        redrawMenuBackground(warningY, warningY + 13);
        showWarningTicks = 0;
        SWOS::FlipInMenu();
    }
}

static Uint32 printWarning(QuickConfigStatus status, const char *control, const QuickConfigContext& context, char *warningBuffer, size_t warningBufferSize)
{
    constexpr Uint32 kWarningInterval = 750;

    if (status == QuickConfigStatus::kAlreadyUsed)
        snprintf(warningBuffer, warningBufferSize, "%s '%s' IS ALREADY USED", context.selectWhat, control);
    else
        snprintf(warningBuffer, warningBufferSize, "%s '%s' IS TAKEN BY PLAYER %d", context.selectWhat, control, context.player == kPlayer1 ? 2 : 1);

    auto showWarningTicks = SDL_GetTicks() + kWarningInterval;

    redrawMenuBackground(context.warningY, context.warningY + 13);
    drawMenuTextCentered(kVgaWidth / 2, context.warningY, warningBuffer, -1, kYellowText);
    SWOS::FlipInMenu();

    return showWarningTicks;
}
