#pragma once

#pragma pack(push, 1)
struct RgbQuad {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
};
#pragma pack(pop)

bool saveBmp8Bit(const char *path, int width, int height, const char *data, const RgbQuad *palette, int numColors);
