#include "redefineKeysMenu.h"
#include "keyBuffer.h"
#include "keyboard.h"
#include "controls.h"
#include "gameControlEvents.h"
#include "quickConfigMenu.h"

static DefaultScancodesPack m_keys;

static QuickConfigStatus getScanCodeStatus(PlayerNumber player, SDL_Scancode key, int numKeys = 0)
{
    assert(player == kPlayer1 || player == kPlayer2);

    if (std::find(m_keys.begin(), m_keys.begin() + numKeys, key) != m_keys.begin() + numKeys)
        return QuickConfigStatus::kAlreadyUsed;

    if (player == kPlayer1 && getPl2Controls() == kKeyboard2) {
        if (playerHasScancode(kPlayer2, key))
            return QuickConfigStatus::kTakenByOtherPlayer;
    } else if (player == kPlayer2 && getPl1Controls() == kKeyboard1) {
        if (playerHasScancode(kPlayer1, key))
            return QuickConfigStatus::kTakenByOtherPlayer;
    }

    return QuickConfigStatus::kContinue;
}

static void resetKeys(QuickConfigContext& context)
{
    m_keys.fill(SDL_SCANCODE_UNKNOWN);
    context.currentSlot = 0;
}

static void addKey(QuickConfigContext& context, SDL_Scancode key)
{
    m_keys[context.currentSlot] = key;
    auto& elementName = context.elementNames[context.currentSlot];
    scancodeToString(key, elementName, sizeof(elementName));
    context.currentSlot++;
}

static std::pair<QuickConfigStatus, const char *> getControlKey(QuickConfigContext& context)
{
    auto key = getKeyInterruptible();

    if (key.first)
        return { QuickConfigStatus::kAbort, nullptr };

    if (key.second != SDL_SCANCODE_UNKNOWN) {
        waitForKeyboardAndMouseIdle();

        auto status = getScanCodeStatus(context.player, key.second, context.currentSlot);
        if (status == QuickConfigStatus::kContinue) {
            addKey(context, key.second);
            status = context.currentSlot >= kNumDefaultGameControlEvents ? QuickConfigStatus::kDone : status;
            return { status, nullptr };
        } else {
            auto keyName = scancodeToString(key.second, context.scratchBuf, sizeof(context.scratchBuf));
            return { status, keyName };
        }
    }

    return { QuickConfigStatus::kNoInput, nullptr };
}

SDL_Scancode getControlKey(PlayerNumber player, int warningY)
{
    auto context = QuickConfigContext::getKeyboardContext(player, getControlKey, nullptr);
    context.currentSlot = 0;
    context.warningY = warningY;

    auto status = getNextControl(context);

    return status == QuickConfigStatus::kContinue ? m_keys[0] : SDL_SCANCODE_UNKNOWN;
}

std::pair<bool, DefaultScancodesPack> promptForDefaultKeys(PlayerNumber player)
{
    waitForKeyboardAndMouseIdle();

    auto context = QuickConfigContext::getKeyboardContext(player, getControlKey, resetKeys);
    auto result = promptForDefaultGameEvents(context);

    return { result, m_keys };
}
