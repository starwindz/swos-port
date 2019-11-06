#include "replayOptions.h"
#include "options.h"

static int m_autoSaveReplays;

const char kReplaySection[] = "replays";

static const std::array<Option<int>, 1> kReplayOptions = {
    "autoSaveReplays", &m_autoSaveReplays, 0, 1, 0,
};

void loadReplayOptions(const CSimpleIni& ini)
{
    loadOptions(ini, kReplayOptions, kReplaySection);
}

void saveReplayOptions(CSimpleIni& ini)
{
    saveOptions(ini, kReplayOptions, kReplaySection);
}

bool getAutoSaveReplays()
{
    return m_autoSaveReplays != 0;
}

void setAutoSaveReplays(bool autoSave)
{
    m_autoSaveReplays = autoSave;
}
