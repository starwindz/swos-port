#pragma once

void drawMenuText(int x, int y, const char *str, int maxWidth = INT_MAX, int color = kWhiteText2, bool bigFont = false);
void drawMenuTextRightAligned(int x, int y, const char *str, int maxWidth = INT_MAX, int color = kWhiteText2, bool bigFont = false);
// x coordinate marks the center of the string
void drawMenuTextCentered(int x, int y, const char *str, int maxWidth = INT_MAX, int color = kWhiteText2, bool bigFont = false);

int getStringPixelLength(const char *str, bool bigFont = false);
void elideString(char *str, int maxStrLen, int maxPixels, bool bigFont = false);
void toUpper(char *str);

inline bool isLower(char c)
{
    return c >= 'a' && c <= 'z';
}

inline bool isUpper(char c)
{
    return c >= 'A' && c <= 'Z';
}
