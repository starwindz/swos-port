#include "keyBuffer.h"

static std::deque<std::pair<SDL_Scancode, SDL_Keymod>> m_keyBuffer;

void registerKey(SDL_Scancode scanCode)
{
    m_keyBuffer.push_back({ scanCode, SDL_GetModState() });
}

SDL_Scancode getKey()
{
    return getKeyAndModifier().first;
}

std::pair<SDL_Scancode, SDL_Keymod> getKeyAndModifier()
{
    auto keyAndModifier = std::make_pair(SDL_SCANCODE_UNKNOWN, KMOD_NONE);

    if (!m_keyBuffer.empty()) {
        keyAndModifier = m_keyBuffer.front();
        m_keyBuffer.pop_front();
    }

    return keyAndModifier;
}

size_t numKeysInBuffer()
{
    return m_keyBuffer.size();
}

void flushKeyBuffer()
{
    m_keyBuffer.clear();
}

bool isLastKeyPressed(SDL_Scancode scanCode)
{
    if (m_keyBuffer.empty())
        return false;

    return m_keyBuffer.back().first == scanCode;
}
