#include "file.h"
#include "util.h"
#include "audio.h"
#include "music.h"
#include "render.h"
#include "options.h"
#include "replays.h"
#include "sprites.h"
#include "pitch.h"
#include "controls.h"
#include "menuControls.h"
#include "menuBackground.h"
#include "mainMenu.h"

#ifndef SWOS_TEST
static_assert(offsetof(SwosVM::SwosVariables, g_selectedTeams) -
    offsetof(SwosVM::SwosVariables, competitionFileBuffer) == 5'191, "Load competition buffer broken");
#endif

#ifdef NDEBUG
constexpr int kSentinelSize = 0;
#else
constexpr char kSentinelMagic[] = "nick.slaughter@key.mariah.com";
constexpr int kSentinelSize = sizeof(kSentinelMagic);
#endif

// sizes of all SWOS buffers in DOS and extended memory
constexpr int kDosMemBufferSize = 167'682 + kSentinelSize;
// allocate buffer to hold every SWOS sprite (but only structs, no data)
constexpr int kSpritesBuffer = kNumSprites * sizeof(SpriteGraphics) + kSentinelSize;

constexpr int kExtendedMemoryBufferSize = 393'216 + kSentinelSize;
SDL_UNUSED constexpr int kTotalExtraMemorySize = kDosMemBufferSize + kSpritesBuffer + kExtendedMemoryBufferSize;

#ifdef DEBUG
void verifyBlock(const char *array, size_t size)
{
    assert(!memcmp(array + size - kSentinelSize, kSentinelMagic, kSentinelSize));
}

void checkMemory()
{
    auto extraMemStart = SwosVM::getExtraMemoryArea();

    assert(swos.dosMemOfs30000h == extraMemStart);
    verifyBlock(extraMemStart, kDosMemBufferSize);

    assert(*swos.g_spriteGraphicsPtr == reinterpret_cast<SpriteGraphics *>(extraMemStart + kDosMemBufferSize));
    verifyBlock(extraMemStart + kDosMemBufferSize, kSpritesBuffer);

    assert(swos.linAdr384k == extraMemStart + kDosMemBufferSize + kSpritesBuffer);
    verifyBlock(extraMemStart + kDosMemBufferSize + kSpritesBuffer, kExtendedMemoryBufferSize);
}
#endif

static void printIntroString()
{
    auto str = swos.aSensibleWorldOfSoccerV2_0;
    while (*str != '\r')
        std::cout << *str++;

    std::cout << "[Win32 Port]\n";
}

static void setupExtraMemory()
{
    // make sure we have enough but don't waste too much either ;)
    assert(SwosVM::kExtendedMemSize >= kTotalExtraMemorySize && kTotalExtraMemorySize + 10'000 > SwosVM::kExtendedMemSize);

    auto extraMemStart = SwosVM::getExtraMemoryArea();
    assert(reinterpret_cast<uintptr_t>(extraMemStart) % sizeof(void *) == 0);

    swos.dosMemOfs30000h = extraMemStart;
    swos.dosMemOfs4fc00h = extraMemStart + 0xd400;  // names don't match offsets anymore, but are left for historical preservation ;)
    swos.dosMemOfs60c00h = extraMemStart + 0x1e400;

    *swos.g_spriteGraphicsPtr = reinterpret_cast<SpriteGraphics *>(extraMemStart + kDosMemBufferSize);

    swos.linAdr384k = extraMemStart + kDosMemBufferSize + kSpritesBuffer;

    swos.g_memAllOk = 1;
    swos.g_gotExtraMemoryForSamples = 1;

#ifdef DEBUG
    SwosVM::initSafeMemoryAreas();
    memcpy(swos.dosMemOfs30000h + kDosMemBufferSize - kSentinelSize, kSentinelMagic, kSentinelSize);
    memcpy((*swos.g_spriteGraphicsPtr).asCharPtr() + kSpritesBuffer - kSentinelSize, kSentinelMagic, kSentinelSize);
    memcpy(swos.linAdr384k + kExtendedMemoryBufferSize - kSentinelSize, kSentinelMagic, kSentinelSize);
#endif
}

static void init()
{
    logInfo("Initializing the game...");

    swos.skipIntro = 0;
    printIntroString();

    swos.setupDatBuffer = 0;    // disable this permanently

    logInfo("Setting up base and extended memory pointers");
    setupExtraMemory();

    initAudio();
    initReplays();
    initMenuBackground();
    initSprites();
    initPitches();
}

static void initRandomSeed()
{
    auto tm = getCurrentTime();
    auto hundredths = tm.msec / 10;

    auto seed = static_cast<int>(hundredths * hundredths);
    auto lo = (seed & 0xff) ^ tm.min;
    seed = lo | (seed & 0xff00);

    assert(seed <= 0xffff);
    swos.randomSeed = seed;
}

void startMainMenuLoop()
{
    init();

    swos.g_pl1ControlFlags = 0;
    swos.g_pl2ControlFlags = 0;
    swos.pl1Direction = -1;
    swos.pl2Direction = -1;

    resetControls();

    startMenuSong();
    initRandomSeed();

    // must keep it for now, but it's a candidate for removal
    swos.useIndividualPlayerSkinColor = 1;

    swos.goalBasePtr = swos.currentHilBuffer;
    swos.nextGoalPtr = reinterpret_cast<dword *>(swos.hilFileBuffer);

    D0 = 0;
    Randomize2();

    swos.screenWidth = kVgaWidth;

    // flush controls
    SDL_FlushEvents(SDL_KEYDOWN, SDL_KEYUP);

#ifndef NDEBUG
    // verify that the string offsets are OK
    assert(!memcmp(swos.aChairmanScenes + 0x4dc1, "YUGOSLAVIA", 11));
    assert(!memcmp(swos.aChairmanScenes + 0x2e5, "THE PRESIDENT", 14));
    assert(!memcmp(swos.aChairmanScenes + 0x2f, "RETURN TO GAME", 15));
    assert(!memcmp(swos.aChairmanScenes + 0x30c, "EXCELLENT JOB", 13));
    assert(!memcmp(swos.aChairmanScenes + 0x58c3, "QUEENSLAND", 11));
    assert(!memcmp(swos.aChairmanScenes + 0x331, "APPRECIATE IT", 14));
    assert(!memcmp(swos.aChairmanScenes + 0x3d1c, "DISK FULL", 9));
#endif

#ifndef SWOS_TEST
    logInfo("Going to main menu");
    showMainMenu();
#endif
}
