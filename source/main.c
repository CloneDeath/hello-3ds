#include "game.h"
#include "render.h"
#include <3ds.h>
#include <ctype.h>
#include <stdio.h>

static uint32_t topPixels[400 * 240], bottomPixels[320 * 240];
static unsigned buttons(u32 keys) {
    unsigned b = 0;
    if (keys & KEY_LEFT)
        b |= BTN_LEFT;
    if (keys & KEY_RIGHT)
        b |= BTN_RIGHT;
    if (keys & KEY_UP)
        b |= BTN_UP;
    if (keys & KEY_DOWN)
        b |= BTN_DOWN;
    if (keys & (KEY_A | KEY_B))
        b |= BTN_JUMP;
    if (keys & (KEY_X | KEY_Y))
        b |= BTN_ATTACK;
    if (keys & KEY_A)
        b |= BTN_CONFIRM;
    if (keys & KEY_B)
        b |= BTN_CANCEL;
    if (keys & KEY_START)
        b |= BTN_PAUSE;
    if (keys & KEY_SELECT)
        b |= BTN_MAP;
    return b;
}
static void present(gfxScreen_t screen, const uint32_t *pixels, int width) {
    u8 *fb = gfxGetFramebuffer(screen, GFX_LEFT, NULL, NULL);
    for (int x = 0; x < width; x++)
        for (int y = 0; y < 240; y++) {
            uint32_t color = pixels[y * width + x];
            unsigned pos = (x * 240 + 239 - y) * 3;
            fb[pos] = color & 255;
            fb[pos + 1] = (color >> 8) & 255;
            fb[pos + 2] = (color >> 16) & 255;
        }
}
static void nameHunter(Game *g) {
    SwkbdState keyboard;
    char raw[64] = {0}, name[16] = {0};
    swkbdInit(&keyboard, SWKBD_TYPE_NORMAL, 2, NAME_LENGTH);
    swkbdSetHintText(&keyboard, "Name your hunter");
    swkbdSetValidation(&keyboard, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetButton(&keyboard, SWKBD_BUTTON_LEFT, "Back", false);
    swkbdSetButton(&keyboard, SWKBD_BUTTON_RIGHT, "Begin", true);
    SwkbdButton result = swkbdInputText(&keyboard, raw, sizeof raw);
    if (result != SWKBD_BUTTON_RIGHT) {
        g->screen = SLOTS;
        return;
    }
    int out = 0;
    for (int i = 0; raw[i] && out < NAME_LENGTH; i++)
        if ((unsigned char)raw[i] < 128 &&
            (isalnum((unsigned char)raw[i]) || raw[i] == ' ' || raw[i] == '-'))
            name[out++] = raw[i];
    while (out > 0 && name[out - 1] == ' ')
        name[--out] = 0;
    gameNew(g, name);
}
int main(void) {
    gfxInitDefault();
    gfxSet3D(false);
    Game game;
    gameInit(&game);
    Canvas top = {400, 240, topPixels}, bottom = {320, 240, bottomPixels};
    u64 last = osGetTime();
    double accumulated = 0;
    while (aptMainLoop() && !game.exitRequested) {
        hidScanInput();
        u32 held = hidKeysHeld(), pressed = hidKeysDown(), released = hidKeysUp();
        Input input = {buttons(held), buttons(pressed), buttons(released), false, 0, 0};
        circlePosition circle;
        hidCircleRead(&circle);
        if (circle.dx < -35)
            input.held |= BTN_LEFT;
        if (circle.dx > 35)
            input.held |= BTN_RIGHT;
        if (pressed & KEY_TOUCH) {
            touchPosition touch;
            hidTouchRead(&touch);
            input.touched = true;
            input.touchX = touch.px;
            input.touchY = touch.py;
        }
        u64 now = osGetTime();
        accumulated += (double)(now - last);
        last = now;
        // Render at vblank, simulate at 60 Hz; do not accelerate after a suspended applet.
        if (accumulated > 100)
            accumulated = 100;
        if (input.pressed || input.released || input.touched)
            accumulated = accumulated < 16.6667 ? 16.6667 : accumulated;
        while (accumulated >= 16.6667) {
            gameTick(&game, input);
            input.pressed = input.released = 0;
            input.touched = false;
            accumulated -= 16.6667;
        }
        renderGame(&game, top, bottom);
        present(GFX_TOP, topPixels, 400);
        present(GFX_BOTTOM, bottomPixels, 320);
        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
        if (game.screen == NAME_ENTRY) {
            nameHunter(&game);
            last = osGetTime();
            accumulated = 0;
        }
    }
    gfxExit();
    return 0;
}
