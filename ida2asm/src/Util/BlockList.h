#pragma once

#include "DynaArray.h"

template<class T>
class BlockList
{
public:
    BlockList(size_t initialCapacity) : m_data(initialCapacity) {}
    void add(const T& t) {
        auto size = t.size();
        auto buf = m_data.add(size);
        memcpy(buf, &t, size);
    }
    template<typename... Args>
    void emplace(Args... args) {
        auto size = T::requiredSize(args...);
        auto buf = m_data.add(size);
        new (buf) T(args...);
    }
    BlockList& operator<<(const T& t) {
        add(t);
        return *this;
    }

    Util::Iterator<T> begin() const {
        return Util::Iterator<T>(reinterpret_cast<T *>(m_data.begin()));
    }
    Util::Iterator<T> end() const {
        return Util::Iterator<T>(reinterpret_cast<T *>(m_data.end()));
    }

private:
    DynaArray m_data;
};
