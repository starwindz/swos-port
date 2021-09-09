#pragma once

void initTimer();
void initFrameTicks();
void timerProc();
void markFrameStartTime();
void frameDelay(double factor = 1.0);
void measureRendering(std::function<void()> render);
