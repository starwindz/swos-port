#include "SetupKeyboardMenuTest.h"

#define SWOS_STUB_MENU_DATA
#include "setupKeyboard.mnu.h"

void SetupKeyboardMenuTest::init()
{
}

void SetupKeyboardMenuTest::finish()
{
}

void SetupKeyboardMenuTest::defaultCaseInit()
{
}

const char *SetupKeyboardMenuTest::name() const
{
    return "setup-keyboard-menu";
}

const char *SetupKeyboardMenuTest::displayName() const
{
    return "setup keyboard menu";
}

auto SetupKeyboardMenuTest::getCases() -> CaseList
{
    return CaseList();
}
