#include "overlay.h"
#include "windowManager.h"
#include "game.h"
#include "pitch.h"
#include "text.h"
#include "util.h"

constexpr int kShowZoomSolid = 900;
constexpr int kFadeZoom = 600;
constexpr int kShowZoomInterval = kShowZoomSolid + kFadeZoom;

constexpr int kInfoX = 290;
constexpr int kFpsY = 4;

void showFps()
{
    if (getShowFps()) {
        constexpr int kNumFramesForFps = 64;

        auto now = SDL_GetPerformanceCounter();

        static Uint64 s_lastFrameTick;
        if (s_lastFrameTick) {
            static std::array<Uint64, kNumFramesForFps> s_renderTimes;
            static int s_renderTimesIndex;

            auto frameTime = now - s_lastFrameTick;

            s_renderTimes[s_renderTimesIndex] = frameTime;
            s_renderTimesIndex = (s_renderTimesIndex + 1) % s_renderTimes.size();
            int numFrames = 0;

            auto totalRenderTime = std::accumulate(s_renderTimes.begin(), s_renderTimes.end(), 0ULL, [&](auto sum, auto current) {
                if (current) {
                    sum += current;
                    numFrames++;
                }
                return sum;
            });

            auto fps = .0;
            if (numFrames)
                fps = 1. / (static_cast<double>(totalRenderTime) / numFrames / SDL_GetPerformanceFrequency());

            char buf[32];
            formatDoubleNoTrailingZeros(fps, buf, sizeof(buf), 2);

            drawText(kInfoX, kFpsY, buf);
        }

        s_lastFrameTick = now;
    }
}

void showZoomFactor()
{
    if (!isMatchRunning())
        return;

    auto now = SDL_GetTicks();
    auto interval = now - getLastZoomChangeTime();

    if (interval < kShowZoomInterval) {
        int alpha = 255;
        if (interval > kShowZoomSolid)
            alpha = 255 * (kFadeZoom - (interval - kShowZoomSolid)) / kFadeZoom;

        int y = kFpsY;
        if (getShowFps())
            y += kSmallFontHeight + 2;

        char buf[32];
        int len = formatDoubleNoTrailingZeros(getZoomFactor(), buf, sizeof(buf), 2);

        buf[len] = 'x';
        buf[len + 1 ] = '\0';

        drawText(kInfoX, y, buf, -1, kWhiteText, false, alpha);
    }
}
