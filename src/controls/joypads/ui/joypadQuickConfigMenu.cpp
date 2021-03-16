#include "joypadQuickConfigMenu.h"
#include "quickConfigMenu.h"
#include "joypads.h"
#include "menus.h"
#include "text.h"
#include "util.h"

constexpr int kAxisJitterLimit = 2'000;

static DefaultJoypadElementList m_controls;
static JoypadElementValueList m_ignoredInputs;

static bool compareElementForIgnore(const JoypadElementValue& el, const JoypadElementValue& ignored)
{
    if (el.type == JoypadElement::kAxis && std::abs(el.axisValue) < kAxisJitterLimit) {
        return true;
    } else if (el.type == ignored.type && el.index == ignored.index) {
        if (el.type == JoypadElement::kAxis) {
            if (std::abs(ignored.axisValue) > kAxisJitterLimit && sgn(el.axisValue) == sgn(ignored.axisValue))
                return true;
        } else if (el.type == JoypadElement::kBall) {
            int value = el.ball.dx;
            int limit = ignored.ball.dx;
            if (el.ball.dy && ignored.ball.dy) {
                value = el.ball.dy;
                limit = ignored.ball.dy;
            }
            if (sgn(value) == sgn(limit) && std::abs(value) <= std::abs(limit))
                return true;
        }
    }

    return false;
}

static bool joypadElementIgnored(const JoypadElementValue& element)
{
    auto it = std::find_if(m_ignoredInputs.begin(), m_ignoredInputs.end(), [&element](const auto& ignoredElement) {
        return compareElementForIgnore(element, ignoredElement);
    });

    return it != m_ignoredInputs.end();
}

static JoypadElementValue getOppositeDirectionControls(const JoypadElementValue& controls)
{
    assert (controls.type == JoypadElement::kAxis || controls.type == JoypadElement::kBall);

    JoypadElementValue oppositeControls = controls;

    if (controls.type == JoypadElement::kAxis) {
        assert(controls.axisValue);
        bool up = controls.axisValue < 0;
        oppositeControls.axisValue = up ? INT16_MAX : INT16_MIN;
    } else {
        assert(controls.ball.dx || controls.ball.dy);
        auto axis = controls.getBallAxis();
        if (axis.first == JoypadElementValue::kBallX) {
            oppositeControls.ball.dx = -axis.second;
            oppositeControls.ball.dy = 0;
        } else {
            oppositeControls.ball.dx = 0;
            oppositeControls.ball.dy = -axis.second;
        }
    }

    return oppositeControls;
}

static void addControl(QuickConfigContext& context, const JoypadElementValue& element)
{
    assert(context.currentSlot < kNumDefaultGameControlEvents);

    auto& mapping = m_controls[context.currentSlot];
    mapping = element;

    // special handling for axis and balls, automatically fill controls for opposite direction
    if ((mapping.type == JoypadElement::kAxis || mapping.type == JoypadElement::kBall) && context.currentSlot < 4) {
        int oppositeDirection = context.currentSlot ^ 1;
        if (m_controls[oppositeDirection].type == JoypadElement::kNone) {
            const auto& oppositeControls = getOppositeDirectionControls(mapping);
            if (!joypadElementIgnored(oppositeControls))
                m_controls[oppositeDirection] = oppositeControls;
        }
    }

    while (context.currentSlot < kNumDefaultGameControlEvents && m_controls[context.currentSlot].type != JoypadElement::kNone)
        context.currentSlot++;
}

static bool aborted()
{
    if (mouseClickAndRelease())
        return true;

    auto keys = SDL_GetKeyboardState(nullptr);
    return keys[SDL_SCANCODE_ESCAPE] != 0;
}

JoypadElementValueList joypadFilteredValues(int joypadIndex)
{
    auto values = joypadElementValues(joypadIndex);

    values.erase(std::remove_if(values.begin(), values.end(), [](const auto& element) {
        return joypadElementIgnored(element);
    }), values.end());

    return values;
}

static void waitUntil(int joypadIndex, std::function<bool()> pred)
{
    while (true) {
        SDL_PumpEvents();

        if (pred())
            break;

        SDL_Delay(50);
    }
}

static void waitUntilIdle(int joypadIndex)
{
    waitUntil(joypadIndex, [joypadIndex]() {
        auto elements = joypadFilteredValues(joypadIndex);
        return elements.empty();
    });
}

static bool anyButtonDown(const JoypadElementValueList& elements)
{
    return std::any_of(elements.begin(), elements.end(), [](const auto& element) { return element.type == JoypadElement::kButton; });
}

static void waitUntilAllButtonsUp(int joypadIndex)
{
    waitUntil(joypadIndex, [joypadIndex]() {
        auto elements = joypadElementValues(joypadIndex);
        return !anyButtonDown(elements);
    });
}

static bool alreadyHaveElement(const JoypadElementValue& value)
{
    assert(value.type != JoypadElement::kNone);

    auto it = std::find_if(m_controls.begin(), m_controls.end(), [&value](const auto& elem) {
        if (elem.type != value.type || elem.index != value.index)
            return false;

        switch (value.type) {
        case JoypadElement::kHat:
            return value.hatMask == elem.hatMask;
        case JoypadElement::kAxis:
            return sgn(value.axisValue) == sgn(elem.axisValue);
        case JoypadElement::kBall:
            assert(!!value.ball.dx != !!value.ball.dy);
            if (value.ball.dx && elem.ball.dx)
                return sgn(value.ball.dx) == sgn(elem.ball.dx);
            else if (value.ball.dy && elem.ball.dy)
                return sgn(value.ball.dy) == sgn(elem.ball.dy);
            else
                return false;
        default:
            return true;
        }
    });

    return it != m_controls.end();
}

static void syncDisplayNames(QuickConfigContext& context)
{
    for (int i = 0; i < kNumDefaultGameControlEvents; i++)
        if (m_controls[i].type != JoypadElement::kNone && !context.elementNames[i][0])
            m_controls[i].toString(context.elementNames[i], sizeof(context.elementNames[i]));
}

static std::pair<QuickConfigStatus, const char *> getJoypadControl(QuickConfigContext& context)
{
    auto values = joypadFilteredValues(context.joypadIndex);
    if (!values.empty()) {
        const auto& value = values.front();
        bool alreadyHave = alreadyHaveElement(value);

        waitUntilIdle(context.joypadIndex);

        if (alreadyHave) {
            return { QuickConfigStatus::kAlreadyUsed, value.toString(context.scratchBuf, sizeof(context.scratchBuf)) };
        } else {
            addControl(context, value);
            syncDisplayNames(context);
            auto status = context.currentSlot >= kNumDefaultGameControlEvents ? QuickConfigStatus::kDone : QuickConfigStatus::kContinue;
            return { status, nullptr };
        }
    }

    auto status = aborted() ? QuickConfigStatus::kAbort : QuickConfigStatus::kNoInput;
    return { status, nullptr };
}

static void resetControls(QuickConfigContext& context)
{
    for (auto& control : m_controls)
        control.type = JoypadElement::kNone;
    context.currentSlot = 0;
}

static bool joypadIdle(int joypadIndex)
{
    for (const auto& value : joypadElementValues(joypadIndex)) {
        if (joypadElementIgnored(value))
            continue;

        if (value.type == JoypadElement::kAxis && std::abs(value.axisValue) < kAxisJitterLimit)
            continue;

        return false;
    }

    return true;
}

static void drawCalibrateMenu()
{
    constexpr int kTextY = kMenuScreenHeight / 2 - 12;

    redrawMenuBackground();

    drawMenuTextCentered(kMenuScreenWidth / 2, kTextY, "WHEN THE CONTROLLER IS IDLE PRESS ANY BUTTON");
    drawMenuTextCentered(kMenuScreenWidth / 2, kTextY + 10, "(MOUSE CLICK/ESCAPE CANCELS)");

    SWOS::FlipInMenu();
}

static bool calibrate(int joypadIndex)
{
    if (!joypadIdle(joypadIndex)) {
        while (true) {
            drawCalibrateMenu();
            SDL_Delay(100);

            processControlEvents();

            if (aborted())
                return false;

            auto elements = joypadElementValues(joypadIndex);
            if (anyButtonDown(elements)) {
                assert(m_ignoredInputs.empty());

                for (const auto& element : elements)
                    if (element.type != JoypadElement::kButton)
                        m_ignoredInputs.push_back(element);

                waitUntilAllButtonsUp(joypadIndex);

                return true;
            }
        }
    }

    return true;
}

std::pair<bool, DefaultJoypadElementList> promptForDefaultJoypadEvents(PlayerNumber player, int joypadIndex)
{
    m_ignoredInputs.clear();

    waitUntilAllButtonsUp(joypadIndex);

    if (calibrate(joypadIndex)) {
        auto context = QuickConfigContext::getJoypadContext(player, joypadIndex, getJoypadControl, resetControls);
        auto result = promptForDefaultGameEvents(context);
        return { result, m_controls };
    } else {
        return {};
    }
}
