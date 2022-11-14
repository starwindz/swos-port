#pragma once

namespace SWOS {
    int randomize(int value);
    int randomize2(int value);
    int rand();
    int rand2();
    void setRandHook(std::function<int()> hook);
}
