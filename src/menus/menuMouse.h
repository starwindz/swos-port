#pragma once

struct MouseWheelEntryGroup {
    MouseWheelEntryGroup(int ordinal, int last, int scrollUpEntry, int scrollDownEntry)
        : first(ordinal), last(last), scrollUpEntry(scrollUpEntry), scrollDownEntry(scrollDownEntry) {}
    int first;
    int last;
    int scrollUpEntry;
    int scrollDownEntry;
};

using MouseWheelEntryGroupList = std::vector<MouseWheelEntryGroup>;

void resetMenuMouseData();
void menuMouseOnAboutToShowNewMenu();
void menuMouseOnOldMenuRestored();

void setMouseWheelEntries(const MouseWheelEntryGroupList& mouseWheelEntries);
void setGlobalWheelEntries(int upEntry = -1, int downEntry = -1);

void updateMouse();
void determineReachableEntries();
bool isEntryReachable(int index);
