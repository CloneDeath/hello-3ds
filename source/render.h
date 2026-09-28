#ifndef NIGHTFALL_RENDER_H
#define NIGHTFALL_RENDER_H
#include "game.h"
typedef struct {
    int width, height;
    uint32_t *pixels;
} Canvas;
void renderGame(const Game *g, Canvas top, Canvas bottom);
void renderGameEye(const Game *g, Canvas top, Canvas bottom, float eye);
#endif
