#include "render.h"
#include "timer.h"
#include "game.h"
#include "windowManager.h"
#include "joypads.h"
#include "VirtualJoypad.h"
#include "dump.h"
#include "file.h"
#include "util.h"
#include "color.h"

static SDL_Renderer *m_renderer;
static Uint32 m_windowPixelFormat;

static bool m_useLinearFiltering;

static bool m_pendingScreenshot;

static void fade(bool out, bool doubleFade = false, SDL_Surface *surface = nullptr, void (*callback)() = nullptr);
static void doMakeScreenshot();
static SDL_Surface *getScreenSurface();

void initRendering()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        sdlErrorExit("Could not initialize SDL");

    auto window = createWindow();
    if (!window)
        sdlErrorExit("Could not create main window");

    m_windowPixelFormat = SDL_GetWindowPixelFormat(window);
    if (m_windowPixelFormat == SDL_PIXELFORMAT_UNKNOWN)
        sdlErrorExit("Failed to query window pixel format");

    m_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!m_renderer)
        sdlErrorExit("Could not create renderer");

    SDL_RendererInfo info;
    SDL_GetRendererInfo(m_renderer, &info);
    logInfo("Renderer \"%s\" created successfully, maximum texture width: %d, height: %d",
        info.name, info.max_texture_width, info.max_texture_height);

    logInfo("Supported texture formats:");
    for (size_t i = 0; i < info.num_texture_formats; i++)
        logInfo("    %s", SDL_GetPixelFormatName(info.texture_formats[i]));

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "best");

#ifdef VIRTUAL_JOYPAD
    getVirtualJoypad().setRenderer(m_renderer);
#endif
}

void finishRendering()
{
    if (m_renderer)
        SDL_DestroyRenderer(m_renderer);

    destroyWindow();

    SDL_Quit();
}

SDL_Renderer *getRenderer()
{
    assert(m_renderer);
    return m_renderer;
}

SDL_Rect getViewport()
{
    SDL_Rect viewport;
    SDL_RenderGetViewport(m_renderer, &viewport);
    return viewport;
}

void updateScreen(bool delay /* = false */)
{
#ifdef VIRTUAL_JOYPAD
    getVirtualJoypad().render(m_renderer);
#endif

#ifdef DEBUG
    dumpVariables();
#endif

    if (m_pendingScreenshot) {
        doMakeScreenshot();
        m_pendingScreenshot = false;
    }

    if (delay)
        frameDelay();

    measureRendering([]() {
        SDL_RenderPresent(m_renderer);
    });

    // must clear the renderer or there will be display artifacts on Samsung phone
    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderClear(m_renderer);
}

void fadeIn()
{
    fade(false);
}

void fadeOut()
{
    fade(true);
}

void fadeInAndOut(void (*callback)() /* = nullptr */)
{
    fade(false, true, nullptr, callback);
}

void drawRectangle(int x, int y, int width, int height, const Color& color)
{
    SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, 255);
    SDL_RenderSetScale(m_renderer, getXScale(), getYScale());
    SDL_Rect dstRect{ x, y, width, height };
    SDL_RenderDrawRect(m_renderer, &dstRect);
    SDL_RenderSetScale(m_renderer, 1, 1);
}

bool getLinearFiltering()
{
    return m_useLinearFiltering;
}

void setLinearFiltering(bool useLinearFiltering)
{
    if (useLinearFiltering != m_useLinearFiltering) {
        if (SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, useLinearFiltering ? "1" : "0"))
            m_useLinearFiltering = useLinearFiltering;
        else
            logWarn("Couldn't %s linear filtering", useLinearFiltering ? "enable" : "disable");
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
    m_pendingScreenshot = true;
}

static void fade(bool out, bool doubleFade /* = false */, SDL_Surface *surface /* = nullptr */, void (*callback)() /* = nullptr */)
{
    constexpr int kFadeDelayMs = 900;
    constexpr int kNumSteps = 255;

    auto start = SDL_GetPerformanceCounter();
    auto freq = SDL_GetPerformanceFrequency();
    auto step = freq * kFadeDelayMs / 1'000 / kNumSteps;

    surface = surface ? surface : getScreenSurface();

    if (surface) {
        if (auto texture = SDL_CreateTextureFromSurface(m_renderer, surface)) {
            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
            for (int i = 0; i < static_cast<int>(doubleFade) + 1; i++) {
                for (int j = 0; j < kNumSteps; j++) {
                    SDL_RenderCopy(m_renderer, texture, nullptr, nullptr);
                    SDL_RenderFillRect(m_renderer, nullptr);
                    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, out ? j + 1 : kNumSteps - j);

                    auto now = SDL_GetPerformanceCounter();
                    if (now < start + step) {
                        auto sleepInterval = (start + step - now) * 1'000 / freq;
                        SDL_Delay(static_cast<Uint32>(sleepInterval));
                        do {
                            now = SDL_GetPerformanceCounter();
                        } while (now < start + step);
                    }
                    start = now;

                    SDL_RenderPresent(m_renderer);
                }
                out = !out;
                if (callback)
                    callback();
            }

            SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_NONE);
            SDL_DestroyTexture(texture);
        } else {
            logWarn("Failed to create a texture for the fade: %s", SDL_GetError());
        }
        SDL_FreeSurface(surface);
    }
}

static void doMakeScreenshot()
{
    char filename[256];

    auto t = time(nullptr);
    auto len = strftime(filename, sizeof(filename), "screenshot-%Y-%m-%d-%H-%M-%S-", localtime(&t));

    auto ms = SDL_GetTicks() % 1'000;
    sprintf(filename + len, "%03d.png", ms);

    const auto& screenshotsPath = ensureScreenshotsDirectory();
    const auto& path = joinPaths(screenshotsPath.c_str(), filename);

    if (auto surface = getScreenSurface()) {
        if (IMG_SavePNG(surface, path.c_str()) >= 0)
            logInfo("Screenshot created: %s", filename);
        else
            logWarn("Failed to save screenshot %s: %s", filename, IMG_GetError());

        SDL_FreeSurface(surface);
    }
}

static SDL_Surface *getScreenSurface()
{
    SDL_Surface *surface = nullptr;
    auto viewPort = getViewport();

    if (surface = SDL_CreateRGBSurfaceWithFormat(0, viewPort.w, viewPort.h, 32, m_windowPixelFormat)) {
        if (SDL_RenderReadPixels(m_renderer, nullptr, m_windowPixelFormat, surface->pixels, surface->pitch) < 0) {
            SDL_FreeSurface(surface);
            surface = nullptr;
            logWarn("Failed to read the pixels from the renderer: %s", SDL_GetError());
        }
    } else {
        logWarn("Failed to create a surface for the screen: %s", SDL_GetError());
    }

    return surface;
}
