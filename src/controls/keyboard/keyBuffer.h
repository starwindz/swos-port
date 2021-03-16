#pragma once

constexpr uint8_t kKeyReleased = 128;

void registerKey(SDL_Scancode scanCode);
SDL_Scancode getKey();
SDL_Scancode peekKey();
size_t numKeysInBuffer();
void flushKeyBuffer();
bool isLastKeyPressed(SDL_Scancode scanCode);
