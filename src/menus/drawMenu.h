#pragma once

static inline void drawMenuItem(MenuEntry *entry)
{
    A5 = entry;
    SWOS::DrawMenuItem();
}
