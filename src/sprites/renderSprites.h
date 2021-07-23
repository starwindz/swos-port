#pragma once

struct PackedSprite;

void drawMenuSprite(int spriteIndex, int x, int y);
void drawCharSprite(int spriteIndex, int x, int y);
const PackedSprite& drawSprite(int pictureIndex, float x, float y, int screenWidth, int screenHeight);
void copySprite(int sourceSpriteIndex, int destSpriteIndex, int xOffset, int yOffset);
