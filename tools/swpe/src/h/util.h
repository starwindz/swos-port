#pragma once

char *int2str(int num, char *buf);
int str2int(uchar *buf);
void my_strncpy(char *dest, const char *src, uint len);

void FillBitmapInfo(BITMAPFILEHEADER *bmfh, BITMAPINFOHEADER *bmih, uint num_colors, uint width, uint height);

uint usqrt(uint n);

char *my_strdup(const char *src);
int my_stricmp(const char *s1, const char *s2);

void Empty(void);
void SortNames(char **names, int n);

#undef toupper
#undef islower
#undef isdigit
#define islower(a) ((a) > 96 && (a) < 123)
#define toupper(a) (islower(a) ? (a) & 0xdf : (a))
#define isdigit(a) ((a) >= '0' && (a) <= '9')

#define isKeyDown(key) ((GetAsyncKeyState(key) & (1 << 31)) != 0)
#define isShiftDown() ((GetAsyncKeyState(VK_SHIFT) & (1 << 31)) != 0)
#define isControlDown() ((GetAsyncKeyState(VK_CONTROL) & (1 << 31)) != 0)
#define isAltDown() ((GetAsyncKeyState(VK_MENU) & (1 << 31)) != 0)

#define swap(a, b) \
    do { \
        unsigned char buf[sizeof(a)]; \
        memcpy(buf, &(a), sizeof(buf)); \
        memcpy(&(a), &(b), sizeof(buf)); \
        memcpy(&(b), buf, sizeof(buf)); \
    } while (0)
