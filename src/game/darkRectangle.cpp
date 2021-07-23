#include "darkRectangle.h"
#include "render.h"

static SDL_Renderer *prepareRendererForDarkenedRectangles();

void drawDarkRectangle(const SDL_FRect& darkRect, int screenWidth, int screenHeight)
{
    auto renderer = prepareRendererForDarkenedRectangles();

    auto width = darkRect.w - darkRect.x;
    auto height = darkRect.h - darkRect.y;

    SDL_FRect rect{ darkRect.x, darkRect.y, width, height };

    SDL_RenderFillRectF(renderer, &rect);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

void darkenRectangles(const SDL_FRect *rects, int numRects)
{
    auto renderer = prepareRendererForDarkenedRectangles();
    SDL_RenderFillRectsF(renderer, rects, numRects);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
}

static SDL_Renderer *prepareRendererForDarkenedRectangles()
{
    auto renderer = getRenderer();
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 127);
    return renderer;
}
