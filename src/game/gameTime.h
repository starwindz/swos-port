#pragma once

void resetTime();
void updateGameTime();
void drawGameTime(const Sprite& sprite);
void drawGameTime(int digit1, int digit2, int digit3);
dword gameTimeInMinutes();
std::tuple<int, int, int> gameTimeAsBcd();
bool gameAtZeroMinute();
