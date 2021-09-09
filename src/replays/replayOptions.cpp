#include "replayOptions.h"
#include "options.h"

static int m_autoSaveReplays;
static int m_showReplayPercentage;

const char kReplaySection[] = "replays";

static const std::array<Option<int>, 2> kReplayOptions = {
    "autoSaveReplays", &m_autoSaveReplays, 0, 1, 0,
    "showReplayPercentage", &m_showReplayPercentage, 0, 1, 0,
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

bool getShowReplayPercentage()
{
    return m_showReplayPercentage != 0;
}

void toggleShowReplayPercentage()
{
    m_showReplayPercentage = !m_showReplayPercentage;
}
