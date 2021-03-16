#include "keyBuffer.h"

static std::deque<SDL_Scancode> m_keyBuffer;

void registerKey(SDL_Scancode scanCode)
{
    m_keyBuffer.push_back(scanCode);
}

SDL_Scancode getKey()
{
    auto key = SDL_SCANCODE_UNKNOWN;

    if (!m_keyBuffer.empty()) {
        key = m_keyBuffer.front();
        m_keyBuffer.pop_front();
    }

    return key;
}

SDL_Scancode peekKey()
{
    return m_keyBuffer.empty() ? SDL_SCANCODE_UNKNOWN : m_keyBuffer.back();
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

    return m_keyBuffer.back() == scanCode;
}
