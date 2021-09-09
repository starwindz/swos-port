#include "spinningLogo.h"
#include "camera.h"
#include "bench.h"
#include "stats.h"

constexpr int kLogoX = 285;
constexpr int kLogoY = 14;
constexpr int kLogoZ = 10'000;

static bool m_enabled;
static int m_frameIndex;

static Sprite m_logo;

void spinBigSLogo()
{
    if (m_enabled) {
        bool logoSpinning = !inBenchMenus() && !showingPostGameStats();
        if (logoSpinning && (swos.stoppageTimer & 2))
            m_frameIndex = (m_frameIndex + 1) & 0x3f;

        m_logo.show();
        m_logo.pictureIndex = kBigSSpriteStart + m_frameIndex / 2;
        m_logo.x = getCameraX() + kLogoX;
        m_logo.y = getCameraY() + kLogoY + kLogoZ;
        m_logo.z = kLogoZ;
    } else {
        m_logo.pictureIndex = -1;
    }
}

void toggleSpinningLogo()
{
    m_enabled = !m_enabled;
}

Sprite *spinningLogoSprite()
{
    return &m_logo;
}
