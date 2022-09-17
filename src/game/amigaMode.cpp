#include "amigaMode.h"
#include "timer.h"

static bool m_enabled;
static bool m_preventDirectionFlip;

bool amigaModeActive()
{
    return m_enabled;
}

void setAmigaModeEnabled(bool enable)
{
    if (enable != m_enabled) {
        if (m_enabled = enable) {
            logInfo("Switching to Amiga game mode");
            swos.penaltiesInterval = 100;
            swos.initalKickInterval = 750;
            swos.goalCameraInterval = 50;
            swos.letPlayerControlCameraInterval = 500;
            swos.playerDownTacklingInterval = 50;
            swos.playerDownHeadingInterval = 50;
            swos.penaltyLockInterval = 50;
            swos.goalkeeperCatchLimitZ = 22;
            swos.hideResultInterval = 50;
            swos.clearResultInterval = 600;
            swos.clearResultHalftimeInterval = 350;
            swos.kKeeperSaveDistance = 24;
            swos.kBallGroundConstant = 16;
            swos.kBallAirConstant = 10;
            swos.kGravityConstant = 4608;

            setTargetFps(kTargetFpsAmiga);
        } else {
            logInfo("Switching to PC game mode");
            swos.penaltiesInterval = 110;
            swos.initalKickInterval = 825;
            swos.goalCameraInterval = 55;
            swos.letPlayerControlCameraInterval = 550;
            swos.playerDownTacklingInterval = 55;
            swos.playerDownHeadingInterval = 55;
            swos.penaltyLockInterval = 55;
            swos.goalkeeperCatchLimitZ = 27;
            swos.hideResultInterval = 55;
            swos.clearResultInterval = 660;
            swos.clearResultHalftimeInterval = 385;
            swos.kKeeperSaveDistance = 16;
            swos.kBallGroundConstant = 13;
            swos.kBallAirConstant = 4;
            swos.kGravityConstant = 3291;

            setTargetFps(kTargetFpsPC);
        }
    }
}

void SWOS::CheckForAmigaModeDirectionFlipBan()
{
    m_preventDirectionFlip = false;
    if (amigaModeActive()) {
        auto sprite = A5.as<Sprite *>();
        m_preventDirectionFlip = sprite->x.whole() >= 273 && sprite->x.whole() <= 398 &&
            (sprite->y.whole() <= 158 || sprite->y.whole() >= 740);
        SwosVM::flags.zero = !m_preventDirectionFlip;
    }
}

void SWOS::WriteAmigaModeDirectionFlip()
{
    if (m_preventDirectionFlip) {
        auto team = A6.as<TeamGeneralInfo *>();
        team->currentAllowedDirection = -1;
        D0.lo16 = -1;
        SwosVM::ax = -1;
    }
}
