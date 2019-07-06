#pragma once

#pragma pack(push, 1)
struct WaveFileHeader {
    dword id;
    dword size;
    dword format;
};

struct WaveFormatHeader {
    dword id;
    dword size;
    word format;
    word channels;
    dword frequency;
    dword byteRate;
    word sampleSize;
    word bitsPerSample;
};

struct WaveDataHeader {
    dword id;
    dword length;
};
#pragma pack(pop)

constexpr int kSizeofWaveHeader = sizeof(WaveFileHeader) + sizeof(WaveFormatHeader) + sizeof(WaveDataHeader);
