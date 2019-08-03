#include "mockUtil.h"
#include "util.h"

#define getRandomInRange getRandomInRange_REAL
#include "util.cpp"
#undef getRandomInRange

static int m_lastMin = -1, m_lastMax = -1;
static int m_counter;

void resetRandomInRange()
{
    m_counter = 0;
}

int getRandomInRange(int min, int max)
{
    if (min > max)
        std::swap(min, max);

    if (min != m_lastMin || max != m_lastMax) {
        m_counter = 0;
        m_lastMin = min;
        m_lastMax = max;
    }

    if (min == max)
        return min;

    int result = min + m_counter++;
    if (result > max)
        result %= (max - min + 1);

    return result;
}
