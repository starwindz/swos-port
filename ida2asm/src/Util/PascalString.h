#pragma once

class PascalString
{
public:
    PascalString(const char *str, size_t len) {
        Util::assignSize(m_length, len);
        memcpy(data(), str, len);
    }
    static size_t requiredSize(const char *, size_t stringLength) {
        return sizeof(PascalString) + stringLength;
    }
    size_t length() const { return m_length; }
    char *data() const { return (char *)(this + 1); }
    bool empty() const { return m_length == 0; }

private:
    uint8_t m_length;
};
