#include "renderSprites.h"
#include "PackedSprite.h"
#include "sprites.h"
#include "color.h"

static void drawMenuSprite(int spriteIndex, int x, int y, bool resetColor)
{
    if (resetColor)
        setMenuSpritesColor({ 255, 255, 255 });

    int screenWidth, screenHeight;
    std::tie(screenWidth, screenHeight) = getWindowSize();

    drawSprite(spriteIndex, static_cast<float>(x), static_cast<float>(y), screenWidth, screenHeight);
}

void drawMenuSprite(int spriteIndex, int x, int y)
{
    drawMenuSprite(spriteIndex, x, y, true);
}

void drawCharSprite(int spriteIndex, int x, int y)
{
    drawMenuSprite(spriteIndex, x, y, false);
}

const PackedSprite& drawSprite(int pictureIndex, float x, float y, int screenWidth, int screenHeight)
{
    const auto& sprite = getSprite(pictureIndex);

    auto xScale = getXScale();
    auto yScale = getYScale();

    x = x - sprite.centerXF + sprite.xOffsetF;
    auto xDest = x * xScale;
    y = y - sprite.centerYF + sprite.yOffsetF;
    auto yDest = y * yScale;

    if (sprite.rotated)
        std::swap(xScale, yScale);

    auto destWidth = sprite.widthF * xScale;
    auto destHeight = sprite.heightF * yScale;

    auto texture = getTexture(sprite);
    auto renderer = getRenderer();

    SDL_FRect dst{ xDest, yDest, destWidth, destHeight };

    if (sprite.rotated) {
        dst.x += dst.h / 2 - dst.w / 2;
        dst.y += dst.w / 2 - dst.h / 2;
        SDL_RenderCopyExF(renderer, texture, &sprite.frame, &dst, -90.0, nullptr, SDL_FLIP_NONE);
    } else {
        SDL_RenderCopyF(renderer, texture, &sprite.frame, &dst);
    }

    return sprite;
}

void copySprite(int sourceSpriteIndex, int destSpriteIndex, int xOffset, int yOffset)
{
//    auto srcSprite = getSprite(sourceSpriteIndex);
//    auto dstSprite = getSprite(destSpriteIndex);
//
//    assert(xOffset + srcSprite->width <= dstSprite->width);
//    assert(yOffset + srcSprite->height <= dstSprite->height);
//
//    auto src = srcSprite->data;
//    auto dst = dstSprite->data + yOffset * dstSprite->bytesPerLine() + xOffset / 2;
//
//    for (int i = 0; i < srcSprite->height; i++) {
//        for (int j = 0; j < srcSprite->width; j++) {
//            int index = j / 2;
//            dst[index] = src[index] & 0xf0;
//
//            if (++j >= srcSprite->width)
//                break;
//
//            dst[index] |= src[index] & 0x0f;
//        }
//
//        src += srcSprite->bytesPerLine();
//        dst += dstSprite->bytesPerLine();
//    }
}

// Copies source sprite into destination sprite with x and y offsets. Destination sprite must be big enough.
// in:
//      D0 = src sprite
//      D1 = x offset in dest sprite
//      D2 = y offset in dest sprite
//      D3 = dest sprite
//
// Note: x offset can only start from even number (rounded down).
//
void SWOS::CopySprite()
{
    copySprite(D0.asWord(), D3.asWord(), D1.asInt16(), D2.asInt16());
}
