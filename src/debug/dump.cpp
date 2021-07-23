#ifndef NDEBUG

#include "dump.h"
#include "text.h"

static bool m_debugOutput = true;

void dumpVariables()
{
    if (m_debugOutput) {
//        char buf[256];
//        snprintf(buf, sizeof(buf), "%hd", swos.g_benchXLimit);
//        drawText(0, 16, buf, -1, kBlueText);
    }
}

void toggleDebugOutput()
{
    m_debugOutput ^= 1;
}

#endif
