#include "SelectFilesMenuTest.h"

static SelectFilesMenuTest t;

void SelectFilesMenuTest::init() {}
void SelectFilesMenuTest::finish() {}
void SelectFilesMenuTest::defaultCaseInit() {}

const char *SelectFilesMenuTest::name() const
{
    return "select-files-menu";
}

const char *SelectFilesMenuTest::displayName() const
{
    return "select files menu";
}

auto SelectFilesMenuTest::getCases() -> CaseList
{
    return { { "111", "222", nullptr, bind(&SelectFilesMenuTest::test) } };
}
