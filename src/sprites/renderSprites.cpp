#include "renderSprites.h"
#include "PackedSprite.h"
#include "sprites.h"
#include "color.h"

static void drawMenuSprite(int spriteIndex, int x, int y, bool resetColor)
{
    if (resetColor)
        setMenuSpritesColor({ 255, 255, 255 });

    drawSprite(spriteIndex, static_cast<float>(x), static_cast<float>(y));
}

void drawMenuSprite(int spriteIndex, int x, int y)
{
    drawMenuSprite(spriteIndex, x, y, true);
}

void drawCharSprite(int spriteIndex, int x, int y)
{
    drawMenuSprite(spriteIndex, x, y, false);
}

// Returns true if the sprite is at least partially on the screen.
bool drawSprite(int pictureIndex, float x, float y, int screenWidth /* = 0 */, int screenHeight /* = 0 */)
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
        return dst.x < screenWidth && dst.y < screenHeight && dst.x > -dst.h && dst.y > -dst.w;
    } else {
        SDL_RenderCopyF(renderer, texture, &sprite.frame, &dst);
        return dst.x < screenWidth && dst.y < screenHeight && dst.x > -dst.w && dst.y > -dst.h;
    }
}
