#include "random.h"

static byte m_xorIndex;
static byte m_xorKey;

int SWOS::rand()
{
    if (!swos.seed)
        m_xorKey = swos.randomTable[++m_xorIndex];

    return swos.randomTable[swos.seed++] ^ m_xorKey;
}

void SWOS::Rand()
{
    D0 = SWOS::rand();
}
