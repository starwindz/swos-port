#pragma once

constexpr uint8_t kKeyReleased = 128;

void registerKey(SDL_Scancode scanCode);
SDL_Scancode getKey();
std::pair<SDL_Scancode, SDL_Keymod> getKeyAndModifier();
size_t numKeysInBuffer();
void flushKeyBuffer();
bool isLastKeyPressed(SDL_Scancode scanCode);
