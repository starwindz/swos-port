#include "exceptions.h"

static bool m_ignoreAsserts = false;

void SWOS_UnitTest::assertImp(bool expr, const char *exprStr, const char *file, int line)
{
    if (!m_ignoreAsserts && !expr)
        throw AssertionException(exprStr, file, line);
}

bool SWOS_UnitTest::ignoreAsserts(bool ignored)
{
    auto oldState = m_ignoreAsserts;
    m_ignoreAsserts = ignored;
    return oldState;
}
