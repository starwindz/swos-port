#include "bmpWriter.h"

#pragma pack(push, 1)
struct BmpFileHeader {
    char type1;
    char type2;
    int32_t size;
    int16_t reserved1;
    int16_t reserved2;
    int32_t bitsOffset;
};

struct BmpInfoHeader {
    int32_t size;
    int32_t width;
    int32_t height;
    int16_t planes;
    int16_t bitCount;
    int32_t compression;
    int32_t sizeImage;
    int32_t xPelsPerMeter;
    int32_t yPelsPerMeter;
    int32_t colorsUsed;
    int32_t colorsImportant;
};
#pragma pack(pop)

enum BmpCompression {
    RGB = 0,
    RLE8 = 1,
    RLE4 = 2,
    BITFIELDS = 3,
};

static void fillBitmapInfo(BmpFileHeader& fh, BmpInfoHeader& ih, int width, int height, int numColors)
{
    auto size = width * height;

    fh.type1 = 'B';
    fh.type2 = 'M';
    fh.reserved1 = 0;
    fh.reserved2 = 0;
    fh.bitsOffset = sizeof(fh) + sizeof(ih) + numColors * sizeof(RgbQuad);
    fh.size = fh.bitsOffset + size;

    ih.size = sizeof(ih);
    ih.width = width;
    ih.height = height;
    ih.planes = 1;
    ih.bitCount = 8;
    ih.compression = BmpCompression::RGB;
    ih.sizeImage = size;
    ih.xPelsPerMeter = 0;
    ih.yPelsPerMeter = 0;
    ih.colorsUsed = numColors;
    ih.colorsImportant = numColors;
}

bool saveBmp8Bit(const char *path, int width, int height, const char *data, int pitch, const RgbQuad *palette, int numColors)
{
    BmpFileHeader fh;
    BmpInfoHeader ih;

    if (!path || !data || !palette)
        return false;

    auto lineWidth = (width + 3) & ~3;
    auto padding = lineWidth - width;

    fillBitmapInfo(fh, ih, width, height, 256);

    auto file = fopen(path, "wb");
    if (!file)
        return false;

    do {
        if (fwrite(&fh, sizeof(fh), 1, file) != 1)
            break;
        if (fwrite(&ih, sizeof(ih), 1, file) != 1)
            break;
        if (fwrite(palette, sizeof(RgbQuad) * numColors, 1, file) != 1)
            break;

        for (int i = 0; i < height; i++) {
            if (fwrite(data + (height - i - 1) * pitch, width, 1, file) != 1)
                break;
            if (padding && fwrite(data + (height - i - 1) * pitch + width, padding, 1, file) != 1)
                break;
        }

        fclose(file);

        return true;
    } while (false);

    fclose(file);
    remove(path);

    return false;
}
