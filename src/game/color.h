#pragma once

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    bool operator!=(const Color& other) const {
        return r != other.r || g != other.g || b != other.b;
    }
};

struct ColorF {
    float r;
    float g;
    float b;
};

extern const std::array<Color, 16> kMenuPalette;
extern const std::array<Color, 16> kTeamPalette;
extern const std::array<Color, 16> kGamePalette;

enum GamePalette
{
    kGameColorGray = 1,
    kGameColorWhite = 2,
    kGameColorBlack = 3,
    kGameColorGreenishBlack = 8,
    kGameColorRed = 10,
    kGameColorBlue = 11,
    kGameColorGreen = 14,
    kGameColorYellow = 15,
};
