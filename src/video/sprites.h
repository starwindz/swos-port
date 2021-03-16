#pragma once

struct SpriteClipper {
    int x;
    int y;
    int width;
    int height;
    const unsigned char *from;
    int pitch;
    bool odd;

    SpriteClipper(int spriteIndex, int x, int y);
    SpriteClipper(const SpriteGraphics *sprite, int x, int y);
    bool clip();

private:
    void init(const SpriteGraphics *sprite, int x, int y);
};

SpriteGraphics *getSprite(int index);
void drawTeamNameSprites(int spriteIndex, int x, int y);
void drawSprite(int spriteIndex, int x, int y, bool saveSpritePixelsFlag = true);
int drawCharSprite(int spriteIndex, int x, int y, int color);
void drawSpriteUnclipped(SpriteClipper& c, bool saveSpritePixelsFlag = true);
void copySprite(int sourceSpriteIndex, int destSpriteIndex, int xOffset, int yOffset);
