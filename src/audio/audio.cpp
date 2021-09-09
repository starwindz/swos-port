#include "audio.h"
#include "wavFormat.h"
#include "sfx.h"
#include "comments.h"
#include "chants.h"
#include "music.h"
#include "log.h"
#include "swos.h"
#include "options.h"

static int16_t m_musicEnabled;
static int16_t m_soundEnabled;
static int16_t m_commentaryEnabled;

#include "audioOptions.mnu.h"

constexpr int kMaxVolume = MIX_MAX_VOLUME;
constexpr int kMinVolume = 0;

static int16_t m_volume = 100;          // master sound volume
static int16_t m_musicVolume = 100;

static int m_actualFrequency;
static int m_actualChannels;

static void verifySpec(int frequency, int format, int channels)
{
    Uint16 actualFormat;

    Mix_QuerySpec(&m_actualFrequency, &actualFormat, &m_actualChannels);
    logInfo("Audio initialized at %dkHz, format: %d, number of channels: %d", m_actualFrequency, actualFormat, m_actualChannels);

    if (m_actualFrequency != frequency || actualFormat != format || m_actualChannels < channels)
        logWarn("Didn't get the desired audio specification, asked for frequency: %d, format: %d, channels: %d",
            frequency, format, channels);
}

static void resetMenuAudio()
{
    Mix_CloseAudio();

    if (Mix_OpenAudio(kMenuFrequency, MIX_DEFAULT_FORMAT, 2, kMenuChunkSize))
        sdlErrorExit("SDL Mixer failed to initialize for the menus");

    verifySpec(kMenuFrequency, MIX_DEFAULT_FORMAT, 2);

    synchronizeMixVolume();
}

void resetGameAudio()
{
    Mix_CloseAudio();

    if (Mix_OpenAudio(kGameFrequency, MIX_DEFAULT_FORMAT, 1, kGameChunkSize))
        sdlErrorExit("SDL Mixer failed to initialize for the game");

    Mix_Volume(-1, m_volume);
    verifySpec(kGameFrequency, MIX_DEFAULT_FORMAT, 1);
}

void ensureMenuAudioFrequency()
{
    int frequency;
    if (Mix_QuerySpec(&frequency, nullptr, nullptr) && frequency != kMenuFrequency)
        resetMenuAudio();
}

void initAudio()
{
    if (m_soundEnabled)
        resetMenuAudio();
}

void finishAudio()
{
    finishMusic();
    Mix_CloseAudio();
}

void stopAudio()
{
    // SDL_mixer will crash if we call these before it's initialized
    if (Mix_QuerySpec(nullptr, nullptr, nullptr)) {
        Mix_HaltChannel(-1);
        Mix_HaltMusic();
    }
}

static void channelFinished(int channel);

// Called when switching from menus to the game.
void initGameAudio()
{
    if (!m_soundEnabled)
        return;

    waitForMusicToFadeOut();
    resetGameAudio();

    Mix_ChannelFinished(channelFinished);

    initCommentsBeforeTheGame();
    initSfxBeforeTheGame();
    initChantsBeforeTheGame();
}

static void channelFinished(int channel)
{
    if (!commenteryOnChannelFinished(channel) && !chantsOnChannelFinished(channel))
        logDebug("Channel %d finished playing", channel);
}

bool soundEnabled()
{
    return m_soundEnabled != 0;
}

void setSoundEnabled(bool enabled)
{
    m_soundEnabled = enabled;
}

bool musicEnabled()
{
    return m_musicEnabled != 0;
}

void setMusicEnabled(bool enabled)
{
    m_musicEnabled = enabled;
}

bool commentaryEnabled()
{
    return m_commentaryEnabled != 0;
}

void setCommentaryEnabled(bool enabled)
{
    m_commentaryEnabled = enabled;
}

//
// option variables
//

static const std::array<Option<int16_t>, 4> kAudioOptions = {{
    { "sound", &m_soundEnabled, 0, 1, 1 },
    { "music", &m_musicEnabled, 0, 1, 1 },
    { "commentary", &m_commentaryEnabled, 0, 1, 1 },
    { "crowdChants", &m_crowdChantsEnabled, 0, 1, 1 },
}};

const char kAudioSection[] = "audio";
const char kMasterVolume[] = "masterVolume";
const char kMusicVolume[] = "musicVolume";

template <typename T>
static void setVolume(T& dest, int volume, const char *desc)
{
    if (volume < kMinVolume || volume > kMaxVolume)
        logWarn("Invalid value given for %s volume (%d), clamping", desc, volume);

    volume = std::min(volume, kMaxVolume);
    volume = std::max(volume, 0);

    dest = volume;
}

static void setMasterVolume(int volume, bool apply = true)
{
    setVolume(m_volume, volume, "master");

    if (apply && m_soundEnabled)
        Mix_Volume(-1, m_volume);
}

int getMusicVolume()
{
    return m_musicVolume;
}

static void setMusicVolume(int volume, bool apply = true)
{
    setVolume(m_musicVolume, volume, "music");

    if (apply && m_soundEnabled && m_musicEnabled)
        Mix_VolumeMusic(m_musicVolume);
}

void loadAudioOptions(const CSimpleIniA& ini)
{
    loadOptions(ini, kAudioOptions, kAudioSection);

    setCrowdChantsEnabled(m_crowdChantsEnabled != 0);

    auto volume = ini.GetLongValue(kAudioSection, kMasterVolume, 100);
    setMasterVolume(volume, false);

    auto musicVolume = ini.GetLongValue(kAudioSection, kMusicVolume, 100);
    setMusicVolume(musicVolume, false);
}

void saveAudioOptions(CSimpleIni& ini)
{
    m_crowdChantsEnabled = areCrowdChantsEnabled();

    saveOptions(ini, kAudioOptions, kAudioSection);

    ini.SetLongValue(kAudioSection, kMasterVolume, m_volume);
    ini.SetLongValue(kAudioSection, kMusicVolume, m_musicVolume);
}

//
// audio options menu
//

void showAudioOptionsMenu()
{
    showMenu(audioOptionsMenu);
}

static void toggleMasterSound()
{
    m_soundEnabled = !m_soundEnabled;

    if (m_soundEnabled) {
        initAudio();
        if (m_musicEnabled)
            restartMusic();
    } else {
        finishMusic();
        finishAudio();
    }
}

static void toggleMenuMusic()
{
    m_musicEnabled = !m_musicEnabled;

    if (m_musicEnabled)
        restartMusic();
    else
        finishMusic();
}

static void toggleCommentary()
{
    m_commentaryEnabled = !m_commentaryEnabled;
}

static void increaseVolume()
{
    if (m_volume < kMaxVolume)
        setMasterVolume(m_volume + 1);
}

static void decreaseVolume()
{
    if (m_volume > 0)
        setMasterVolume(m_volume - 1);
}

static void volumeBeforeDraw()
{
    auto entry = A5.as<MenuEntry *>();
    entry->fg.number = m_volume;
}

static void increaseMusicVolume()
{
    if (m_musicVolume < kMaxVolume)
        setMusicVolume(getMusicVolume() + 1);
}

static void decreaseMusicVolume()
{
    if (getMusicVolume() > kMinVolume)
        setMusicVolume(getMusicVolume() - 1);
}

static void musicVolumeBeforeDraw()
{
    auto entry = A5.as<MenuEntry *>();
    entry->fg.number = getMusicVolume();
}

static void toggleCrowdChants()
{
    m_crowdChantsEnabled = !m_crowdChantsEnabled;
    setCrowdChantsEnabled(m_crowdChantsEnabled != 0);
}
