#define _GNU_SOURCE
#include "game.h"
#include "render.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void ticks(Game *g, int n, unsigned held) {
    for (int i = 0; i < n; i++)
        gameTick(g, (Input){.held = held});
}
static void press(Game *g, unsigned key) {
    gameTick(g, (Input){.held = key, .pressed = key});
}
static void image(const char *path, Canvas c) {
    FILE *f = fopen(path, "wb");
    assert(f);
    fprintf(f, "P6\n%d %d\n255\n", c.width, c.height);
    for (int i = 0; i < c.width * c.height; i++) {
        uint32_t p = c.pixels[i];
        unsigned char rgb[] = {p >> 16, p >> 8, p};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}
int main(int argc, char **argv) {
    char directory[] = "/tmp/nightfall-test-XXXXXX";
    assert(mkdtemp(directory));
    saveSetDirectory(directory);
    Game g;
    gameInit(&g);
    assert(g.screen == TITLE);
    press(&g, BTN_CONFIRM);
    assert(g.screen == SLOTS);
    press(&g, BTN_CONFIRM);
    assert(g.screen == NAME_ENTRY);
    gameNew(&g, "Nicholas");
    assert(g.screen == PLAY && g.slotUsed[0]);
    float start = g.player.x;
    ticks(&g, 20, BTN_RIGHT);
    assert(g.player.x > start + 30);
    float floor = g.player.y;
    press(&g, BTN_JUMP);
    ticks(&g, 10, BTN_JUMP);
    assert(g.player.y < floor - 40);
    ticks(&g, 65, 0);
    assert(g.player.grounded && fabsf(g.player.y - floor) < 0.1f);
    // One-way platform landing, approached from below with enough horizontal clearance.
    gameEnterRoom(&g, 0, 175);
    press(&g, BTN_JUMP);
    ticks(&g, 60, 0);
    assert(g.player.grounded && g.player.y == 136);
    // Room boundaries update discovery and spawn at the matching entrance.
    gameEnterRoom(&g, 0, 783);
    ticks(&g, 2, BTN_RIGHT);
    assert(g.room == 1 && (g.visited & 2));
    gameEnterRoom(&g, 1, 646);
    ticks(&g, 40, 0);
    press(&g, BTN_UP);
    assert(g.room == 2);
    // Save restores health; loading restores the checkpoint and progression.
    g.player.x = 210;
    g.player.hp = 2;
    g.gold = 17;
    g.kills = 4;
    ticks(&g, 40, 0);
    press(&g, BTN_UP);
    assert(g.player.hp == MAX_HP && g.slots[0].checkpoint == 2);
    g.gold = 999;
    gameLoad(&g, 0);
    assert(g.room == 2 && g.gold == 17 && !strcmp(g.name, "Nicholas"));
    // Pause and map freeze simulation.
    press(&g, BTN_PAUSE);
    float x = g.player.x;
    ticks(&g, 100, BTN_RIGHT);
    assert(g.player.x == x && g.screen == PAUSE);
    press(&g, BTN_CANCEL);
    press(&g, BTN_MAP);
    ticks(&g, 100, BTN_RIGHT);
    assert(g.player.x == x && g.screen == MAP);
    press(&g, BTN_CANCEL);
    // A swing may hit an enemy only once, and three swings kill a skeleton.
    gameEnterRoom(&g, 0, 90);
    Enemy *e = &g.enemies[0];
    e->x = 116;
    e->left = 116;
    e->right = 117;
    e->hp = 3;
    press(&g, BTN_ATTACK);
    ticks(&g, 19, 0);
    assert(e->hp == 2);
    ticks(&g, 12, 0);
    for (int i = 0; i < 2; i++) {
        e->x = 116;
        press(&g, BTN_ATTACK);
        ticks(&g, 32, 0);
    }
    assert(!e->alive && g.kills == 5);
    // Death returns to last committed progress.
    g.player.hp = 1;
    g.player.hurt = 0;
    e->alive = true;
    e->x = g.player.x;
    e->y = g.player.y;
    ticks(&g, 1, 0);
    assert(g.screen == GAME_OVER);
    press(&g, BTN_CONFIRM);
    assert(g.screen == PLAY && g.room == 2 && g.kills == 4);
    // All routes are reversible, including the optional tower.
    gameEnterRoom(&g, 2, 76);
    ticks(&g, 40, 0);
    press(&g, BTN_UP);
    assert(g.room == 1);
    gameEnterRoom(&g, 3, 733);
    ticks(&g, 40, 0);
    press(&g, BTN_UP);
    assert(g.room == 4);
    ticks(&g, 40, 0);
    press(&g, BTN_UP);
    assert(g.room == 3);
    // Separate slots and atomic backup fallback.
    g.slot = 1;
    gameNew(&g, "Alice");
    SaveRecord r;
    assert(saveRead(0, &r) && !strcmp(r.name, "Nicholas"));
    assert(saveRead(1, &r) && !strcmp(r.name, "Alice"));
    gameLoad(&g, 0);
    g.gold = 23;
    assert(gameSave(&g));
    char path[300];
    snprintf(path, sizeof path, "%s/slot1.sav", directory);
    FILE *f = fopen(path, "wb");
    fputs("damaged", f);
    fclose(f);
    assert(saveRead(0, &r) && r.gold == 17);
    // Screenshot the actual software renderer, and render every state and room under ASan.
    static uint32_t top[400 * 240], bottom[320 * 240];
    Canvas tc = {400, 240, top}, bc = {320, 240, bottom};
    if (argc > 1) {
        g.screen = TITLE;
        renderGame(&g, tc, bc);
        char path[512];
        snprintf(path, sizeof path, "%s/title.ppm", argv[1]);
        image(path, tc);
        g.screen = PLAY;
        g.player.hp = MAX_HP;
        gameEnterRoom(&g, 1, 320);
        g.player.y = 148;
        g.player.attack = 12;
        g.visited = 63;
        g.frame = 41;
        g.roomTitleTicks = 0;
        g.player.hurt = 0;
        g.noticeTicks = 0;
        renderGame(&g, tc, bc);
        snprintf(path, sizeof path, "%s/game.ppm", argv[1]);
        image(path, tc);
        snprintf(path, sizeof path, "%s/map.ppm", argv[1]);
        image(path, bc);
        gameEnterRoom(&g, 2, 210);
        g.player.hurt = 0;
        renderGame(&g, tc, bc);
        snprintf(path, sizeof path, "%s/save.ppm", argv[1]);
        image(path, tc);
    }
    for (int room = 0; room < ROOM_COUNT; room++) {
        gameEnterRoom(&g, room, 50);
        for (int screen = TITLE; screen <= QUIT_CONFIRM; screen++) {
            g.screen = screen;
            renderGame(&g, tc, bc);
        }
    }
    puts("PASS: menus, movement, jumping, platforms, doors, combat, death, map, saves, backup, "
         "slot isolation, render bounds.");
    return 0;
}
