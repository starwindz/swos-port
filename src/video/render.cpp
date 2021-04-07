#include "render.h"
#include "windowManager.h"
#include "util.h"
#include "file.h"
#include "dump.h"
#include "joypads.h"
#include "VirtualJoypad.h"
#include <SDL_image.h>

static SDL_Renderer *m_renderer;
static SDL_Texture *m_texture;
static SDL_PixelFormat *m_pixelFormat;

constexpr int kNumColors = 256;
constexpr int kPaletteSize = 3 * kNumColors;
static uint8_t m_palette[kPaletteSize];

static Uint64 m_lastRenderStartTime;
static Uint64 m_lastRenderEndTime;

static bool m_delay;
static std::deque<int> m_lastFramesDelay;
static constexpr int kMaxLastFrames = 16;

void initRendering()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        sdlErrorExit("Could not initialize SDL");

    auto window = createWindow();

    m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
        sdlErrorExit("Could not create renderer");

    auto format = SDL_PIXELFORMAT_RGBA8888;
    m_texture = SDL_CreateTexture(m_renderer, format, SDL_TEXTUREACCESS_STREAMING, kVgaWidth, kVgaHeight);
    if (!m_texture)
        sdlErrorExit("Could not create surface");

    m_pixelFormat = SDL_AllocFormat(format);
    if (!m_pixelFormat)
        sdlErrorExit("Could not allocate pixel format");

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");
    SDL_RenderSetLogicalSize(m_renderer, kVgaWidth, kVgaHeight);

#ifdef VIRTUAL_JOYPAD
    getVirtualJoypad().setRenderer(m_renderer);
#endif
}

void finishRendering()
{
    if (m_pixelFormat)
        SDL_FreeFormat(m_pixelFormat);

    if (m_texture)
        SDL_DestroyTexture(m_texture);

    if (m_renderer)
        SDL_DestroyRenderer(m_renderer);

    destroyWindow();

    SDL_Quit();
}

SDL_Rect getViewport()
{
    SDL_Rect viewport;
    SDL_RenderGetViewport(m_renderer, &viewport);
    return viewport;
}

void setPalette(const char *palette, int numColors /* = kVgaPaletteNumColors */)
{
    assert(numColors >= 0 && numColors <= kVgaPaletteNumColors);

    for (int i = 0; i < numColors * 3; i++) {
        assert(palette[i] >= 0 && palette[i] < 64);
        m_palette[i] = palette[i] * 255 / 63;
    }
}

void getPalette(char *palette)
{
    for (int i = 0; i < kPaletteSize; i++)
        palette[i] = m_palette[i] * 63 / 255;
}

static void determineIfDelayNeeded()
{
    auto delay = m_lastRenderEndTime > m_lastRenderStartTime && m_lastRenderEndTime - m_lastRenderStartTime >= 10;

    if (m_lastFramesDelay.size() >= kMaxLastFrames)
        m_lastFramesDelay.pop_front();

    m_lastFramesDelay.push_back(delay);

    size_t sum = std::accumulate(m_lastFramesDelay.begin(), m_lastFramesDelay.end(), 0);
    m_delay = 2 * sum >= m_lastFramesDelay.size();
}

template <typename F>
static void requestPixelAccess(F drawFunction)
{
    Uint32 *pixels;
    int pitch;

    if (SDL_LockTexture(m_texture, nullptr, (void **)&pixels, &pitch)) {
        logWarn("Failed to lock drawing texture");
        return;
    }

    drawFunction(pixels, pitch);

    SDL_UnlockTexture(m_texture);
}

void clearScreen()
{
    requestPixelAccess([](Uint32 *pixels, int pitch) {
        auto color = &m_palette[0];
        auto rgbaColor = SDL_MapRGBA(m_pixelFormat, color[0], color[1], color[2], 255);

        for (int y = 0; y < kVgaHeight; y++) {
            for (int x = 0; x < kVgaWidth; x++) {
                pixels[y * pitch / sizeof(Uint32) + x] = rgbaColor;
            }
        }
    });
}

void skipFrameUpdate()
{
    m_lastRenderStartTime = m_lastRenderEndTime = SDL_GetPerformanceCounter();
}

void updateScreen(const char *inData /* = nullptr */, int offsetLine /* = 0 */, int numLines /* = kVgaHeight */)
{
    assert(offsetLine >= 0 && offsetLine <= kVgaHeight);
    assert(numLines >= 0 && numLines <= kVgaHeight);
    assert(offsetLine + numLines <= kVgaHeight);

    m_lastRenderStartTime = SDL_GetPerformanceCounter();

    auto data = reinterpret_cast<const uint8_t *>(inData ? inData : (swos.vsPtr ? swos.vsPtr : swos.linAdr384k));

#ifdef DEBUG
    if (swos.screenWidth == kGameScreenWidth)
        dumpVariables();
    SwosVM::verifySafeMemoryAreas();
#endif

    requestPixelAccess([&](Uint32 *pixels, int pitch) {
        for (int y = offsetLine; y < numLines; y++) {
            for (int x = 0; x < kVgaWidth; x++) {
                auto color = &m_palette[data[y * swos.screenWidth + x] * 3];
                pixels[y * pitch / sizeof(Uint32) + x] = SDL_MapRGBA(m_pixelFormat, color[0], color[1], color[2], 255);
            }
        }
    });

    // not clearing the buffer causes artefacts on Samsung phone
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);

    SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
#ifdef VIRTUAL_JOYPAD
    getVirtualJoypad().render(m_renderer);
#endif
    SDL_RenderPresent(m_renderer);

    m_lastRenderEndTime = SDL_GetPerformanceCounter();

    determineIfDelayNeeded();
}

void frameDelay(double factor /* = 1.0 */)
{
    timerProc();

    if (!m_delay) {
        SDL_Delay(3);
        return;
    }

    constexpr double kTargetFps = 70;

    if (factor > 1.0) {
        // don't use busy wait in menus
        auto delay = std::lround(1'000 * factor / kTargetFps);
        SDL_Delay(delay);
        return;
    }

    static const Sint64 kFrequency = SDL_GetPerformanceFrequency();

    Uint64 delay = std::llround(kFrequency * factor / kTargetFps);

    auto startTicks = SDL_GetPerformanceCounter();
    auto diff = startTicks - m_lastRenderStartTime;

    if (diff < delay) {
        constexpr int kNumFramesForSlackValue = 64;
        static std::array<Uint64, kNumFramesForSlackValue> slackValues;
        static int slackValueIndex;

        auto slackValue = std::accumulate(std::begin(slackValues), std::end(slackValues), 0LL);
        slackValue = (slackValue + (slackValue > 0 ? kNumFramesForSlackValue : -kNumFramesForSlackValue) / 2) / kNumFramesForSlackValue;

        if (static_cast<Sint64>(delay - diff) > slackValue) {
            auto intendedDelay = 1'000 * (delay - diff - slackValue) / kFrequency;
            auto delayStart = SDL_GetPerformanceCounter();
            SDL_Delay(static_cast<Uint32>(intendedDelay));

            auto actualDelay = SDL_GetPerformanceCounter() - delayStart;
            slackValues[slackValueIndex] = actualDelay - intendedDelay * kFrequency / 1'000;
            slackValueIndex = (slackValueIndex + 1) % kNumFramesForSlackValue;
        }

        do {
            startTicks = SDL_GetPerformanceCounter();
        } while (m_lastRenderStartTime + delay > startTicks);
    }
}

// simulate SWOS procedure executed at each interrupt 8 tick
void timerProc()
{
    swos.currentTick++;
    swos.menuCycleTimer++;
    if (!swos.paused) {
        swos.stoppageTimer++;
        swos.timerBoolean = (swos.timerBoolean + 1) & 1;
        if (!swos.timerBoolean)
            swos.bumpBigSFrame = -1;
    }
}

void fadeIfNeeded()
{
    if (!swos.skipFade) {
        FadeIn();
        swos.skipFade = -1;
    }
}

std::string ensureScreenshotsDirectory()
{
    auto path = pathInRootDir("screenshots");
    createDir(path.c_str());
    return path;
}

void makeScreenshot()
{
    char filename[256];

    auto t = time(nullptr);
    auto len = strftime(filename, sizeof(filename), "screenshot-%Y-%m-%d-%H-%M-%S-", localtime(&t));

    auto ms = SDL_GetTicks() % 1'000;
    sprintf(filename + len, "%03d.png", ms);

    const auto& screenshotsPath = ensureScreenshotsDirectory();
    const auto& path = joinPaths(screenshotsPath.c_str(), filename);

    requestPixelAccess([&](Uint32 *pixels, int pitch) {
        Uint32 rMask, gMask, bMask, aMask;
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
        rMask = 0xff000000;
        gMask = 0x00ff0000;
        bMask = 0x0000ff00;
        aMask = 0x000000ff;
#else
        rMask = 0x000000ff;
        gMask = 0x0000ff00;
        bMask = 0x00ff0000;
        aMask = 0xff000000;
#endif

        auto surface = SDL_CreateRGBSurfaceFrom(pixels, kVgaWidth, kVgaHeight, 32, pitch, rMask, gMask, bMask, aMask);
        bool surfaceLocked = surface ? SDL_LockSurface(surface) >= 0 : false;

        if (surface && surfaceLocked && IMG_SavePNG(surface, path.c_str()) >= 0)
            logInfo("Created screenshot %s", filename);
        else
            logWarn("Failed to save screenshot %s", filename);

        if (surfaceLocked)
            SDL_UnlockSurface(surface);

        SDL_FreeSurface(surface);
    });
}

void SWOS::Flip()
{
    processControlEvents();
    frameDelay();
    updateScreen();
}
