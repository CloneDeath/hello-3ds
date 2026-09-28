#include "render.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INK 0x0a0d18
#define PAPER 0xe7dfc8
#define GOLD 0xd8b46c
#define MUTED 0x9195a5
#define GREEN 0x76d9ae
#define RED 0xd35972
static const uint8_t font[96][7] = {
    ['A' - 32] = {14, 17, 17, 31, 17, 17, 17}, ['B' - 32] = {30, 17, 17, 30, 17, 17, 30},
    ['C' - 32] = {14, 17, 16, 16, 16, 17, 14}, ['D' - 32] = {30, 17, 17, 17, 17, 17, 30},
    ['E' - 32] = {31, 16, 16, 30, 16, 16, 31}, ['F' - 32] = {31, 16, 16, 30, 16, 16, 16},
    ['G' - 32] = {14, 17, 16, 23, 17, 17, 15}, ['H' - 32] = {17, 17, 17, 31, 17, 17, 17},
    ['I' - 32] = {14, 4, 4, 4, 4, 4, 14},      ['J' - 32] = {7, 2, 2, 2, 18, 18, 12},
    ['K' - 32] = {17, 18, 20, 24, 20, 18, 17}, ['L' - 32] = {16, 16, 16, 16, 16, 16, 31},
    ['M' - 32] = {17, 27, 21, 21, 17, 17, 17}, ['N' - 32] = {17, 25, 25, 21, 19, 19, 17},
    ['O' - 32] = {14, 17, 17, 17, 17, 17, 14}, ['P' - 32] = {30, 17, 17, 30, 16, 16, 16},
    ['Q' - 32] = {14, 17, 17, 17, 21, 18, 13}, ['R' - 32] = {30, 17, 17, 30, 20, 18, 17},
    ['S' - 32] = {15, 16, 16, 14, 1, 1, 30},   ['T' - 32] = {31, 4, 4, 4, 4, 4, 4},
    ['U' - 32] = {17, 17, 17, 17, 17, 17, 14}, ['V' - 32] = {17, 17, 17, 17, 17, 10, 4},
    ['W' - 32] = {17, 17, 17, 21, 21, 21, 10}, ['X' - 32] = {17, 17, 10, 4, 10, 17, 17},
    ['Y' - 32] = {17, 17, 10, 4, 4, 4, 4},     ['Z' - 32] = {31, 1, 2, 4, 8, 16, 31},
    ['0' - 32] = {14, 17, 19, 21, 25, 17, 14}, ['1' - 32] = {4, 12, 4, 4, 4, 4, 14},
    ['2' - 32] = {14, 17, 1, 2, 4, 8, 31},     ['3' - 32] = {30, 1, 1, 14, 1, 1, 30},
    ['4' - 32] = {2, 6, 10, 18, 31, 2, 2},     ['5' - 32] = {31, 16, 16, 30, 1, 1, 30},
    ['6' - 32] = {14, 16, 16, 30, 17, 17, 14}, ['7' - 32] = {31, 1, 2, 4, 8, 8, 8},
    ['8' - 32] = {14, 17, 17, 14, 17, 17, 14}, ['9' - 32] = {14, 17, 17, 15, 1, 1, 14},
    ['.' - 32] = {0, 0, 0, 0, 0, 12, 12},      [',' - 32] = {0, 0, 0, 0, 0, 4, 8},
    [':' - 32] = {0, 4, 4, 0, 4, 4, 0},        ['!' - 32] = {4, 4, 4, 4, 4, 0, 4},
    ['?' - 32] = {14, 17, 1, 2, 4, 0, 4},      ['-' - 32] = {0, 0, 0, 31, 0, 0, 0},
    ['/' - 32] = {1, 1, 2, 4, 8, 16, 16},      ['>' - 32] = {16, 8, 4, 2, 4, 8, 16},
    ['<' - 32] = {1, 2, 4, 8, 4, 2, 1},        ['\'' - 32] = {4, 4, 8, 0, 0, 0, 0},
    ['+' - 32] = {0, 4, 4, 31, 4, 4, 0}};
static void pixel(Canvas c, int x, int y, uint32_t color) {
    if (x >= 0 && x < c.width && y >= 0 && y < c.height)
        c.pixels[y * c.width + x] = color;
}
static void box(Canvas c, int x, int y, int w, int h, uint32_t color) {
    int right = x + w, bottom = y + h;
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (right > c.width)
        right = c.width;
    if (bottom > c.height)
        bottom = c.height;
    for (int yy = y; yy < bottom; yy++)
        for (int xx = x; xx < right; xx++)
            c.pixels[yy * c.width + xx] = color;
}
static void line(Canvas c, int x, int y, int xx, int yy, uint32_t color) {
    int dx = abs(xx - x), sx = x < xx ? 1 : -1, dy = -abs(yy - y), sy = y < yy ? 1 : -1,
        error = dx + dy;
    for (;;) {
        pixel(c, x, y, color);
        if (x == xx && y == yy)
            break;
        int e = error * 2;
        if (e >= dy) {
            error += dy;
            x += sx;
        }
        if (e <= dx) {
            error += dx;
            y += sy;
        }
    }
}
static void border(Canvas c, int x, int y, int w, int h, uint32_t color) {
    box(c, x, y, w, 1, color);
    box(c, x, y + h - 1, w, 1, color);
    box(c, x, y, 1, h, color);
    box(c, x + w - 1, y, 1, h, color);
}
static void disk(Canvas c, int x, int y, int radius, uint32_t color) {
    for (int yy = -radius; yy <= radius; yy++) {
        int half = (int)sqrtf(radius * radius - yy * yy);
        box(c, x - half, y + yy, half * 2 + 1, 1, color);
    }
}
static void text(Canvas c, int x, int y, const char *s, int scale, uint32_t color) {
    for (; *s; s++, x += 6 * scale) {
        unsigned char ch = (unsigned char)toupper((unsigned char)*s);
        if (ch < 32 || ch > 127)
            ch = '?';
        for (int row = 0; row < 7; row++)
            for (int col = 0; col < 5; col++)
                if (font[ch - 32][row] & (1 << (4 - col)))
                    box(c, x + col * scale, y + row * scale, scale, scale, color);
    }
}
static void centered(Canvas c, int y, const char *s, int scale, uint32_t color) {
    text(c, (c.width - (int)strlen(s) * 6 * scale) / 2, y, s, scale, color);
}
static void panel(Canvas c, int x, int y, int w, int h) {
    box(c, x, y, w, h, 0x151a2b);
    border(c, x, y, w, h, 0x62546b);
    border(c, x + 3, y + 3, w - 6, h - 6, 0x2a2a3c);
}
static void backdrop(Canvas c, int frame) {
    for (int y = 0; y < c.height; y++)
        box(c, 0, y, c.width, 1, (uint32_t)(0x0a0e1c + ((y / 28) << 16) + ((y / 28) << 8)));
    for (int i = 0; i < 46; i++)
        pixel(c, (i * 79 + 31) % c.width, (i * 37 + 13) % 130,
              (i + frame / 40) % 5 == 0 ? GOLD : 0x667386);
    disk(c, c.width - 43, 40, 24, 0xbcc7c5);
    disk(c, c.width - 36, 33, 21, 0x152033);
    for (int i = 0; i < 9; i++) {
        int x = i * 55 - 16, h = 52 + (i * 23) % 67;
        box(c, x, 240 - h, 39, h, 0x0d1221);
        for (int j = 0; j < 22; j++)
            box(c, x - 3 + j, 240 - h - 22 + j, 45 - j * 2, 1, 0x0d1221);
        for (int j = 0; j < 3; j++)
            box(c, x + 10, 240 - h + 15 + j * 20, 5, 9, 0x343345);
    }
    box(c, 0, 225, c.width, 15, INK);
}
static void candle(Canvas c, int x, int y, int frame, uint32_t glow) {
    box(c, x - 1, y, 3, 12, 0xc4bda8);
    box(c, x - 3, y + 12, 7, 2, 0x5d5060);
    disk(c, x, y - 3, 3 + (frame / 8 % 2), 0x55432d);
    box(c, x - 1, y - 6, 3, 5, glow);
    pixel(c, x, y - 7, PAPER);
}
static void window(Canvas c, int x, int y, uint32_t accent) {
    box(c, x - 3, y + 13, 40, 78, 0x181926);
    box(c, x, y + 15, 34, 70, 0x263047);
    for (int row = 0; row < 17; row++)
        box(c, x + 16 - row, y + 16 - row, row * 2 + 2, 1, 0x263047);
    box(c, x + 15, y + 2, 3, 84, accent);
    box(c, x, y + 49, 34, 3, accent);
    box(c, x - 4, y + 85, 42, 4, accent);
    line(c, x + 2, y + 30, x + 30, y + 70, 0x465166);
}
static void roomBackground(Canvas c, const Game *g) {
    const Room *r = &rooms[g->room];
    box(c, 0, 0, 400, 240, r->sky);
    if (g->room == 0 || g->room == 3 || g->room == 4) {
        disk(c, 323 - g->camera / 12, 60, 24, 0x818da1);
        for (int i = 0; i < 15; i++) {
            int x = i * 85 - g->camera / 3;
            box(c, x, 124 + (i % 3) * 12, 55, 90, 0x1a2336);
            box(c, x + 15, 104 + (i % 3) * 12, 25, 100, 0x1a2336);
        }
        for (int i = 0; i < 30; i++)
            pixel(c, (i * 73 + 14 - g->camera / 10 + 1000) % 400, 30 + i * 19 % 85, 0x62708e);
    }
    for (int x = -g->camera % 96 - 96; x < 400; x += 96) {
        if (g->room != 3)
            window(c, x + 25, 54, r->accent);
        box(c, x + 3, 28, 13, 180, r->stone);
        box(c, x, 30, 19, 7, r->accent);
        box(c, x + 4, 40, 3, 160, 0x505063);
        box(c, x, 199, 20, 9, r->accent);
    }
    for (int x = -g->camera % 32 - 32; x < 400; x += 32) {
        box(c, x, 208, 32, 32, r->stone);
        box(c, x, 208, 31, 3, r->accent);
        box(c, x, 224, 32, 1, 0x191b2a);
        box(c, x + 30, 212, 2, 12, 0x191b2a);
        box(c, x + 14, 225, 2, 15, 0x191b2a);
        line(c, x + 5, 214, x + 10, 217, 0x252737);
        line(c, x + 10, 217, x + 8, 221, 0x252737);
    }
    for (int i = 0; i < r->platformCount; i++) {
        Platform p = r->platforms[i];
        int x = p.x - g->camera;
        box(c, x, p.y, p.w, 9, r->stone);
        box(c, x, p.y, p.w, 3, r->accent);
        box(c, x + 4, p.y + 9, 5, 5, r->stone);
        box(c, x + p.w - 9, p.y + 9, 5, 5, r->stone);
    }
    if (r->leftRoom < 0) {
        box(c, -g->camera, 110, 11, 98, r->stone);
    }
    if (r->rightRoom < 0)
        box(c, r->width - 11 - g->camera, 110, 11, 98, r->stone);
    if (r->portalX >= 0) {
        int x = r->portalX - g->camera;
        box(c, x - 14, 155, 43, 53, 0x080e19);
        border(c, x - 17, 152, 49, 56, r->accent);
        for (int i = 0; i < 4; i++)
            box(c, x - 10 + i * 9, 158, 2, 48, 0x393548);
        candle(c, x - 26, 179, g->frame, GOLD);
        candle(c, x + 38, 179, g->frame, GOLD);
    }
    if (r->saveRoom) {
        int x = 218 - g->camera;
        disk(c, x, 135, 23, 0x223e3b);
        disk(c, x, 135, 17, 0x2b5148);
        border(c, x - 10, 125, 21, 21, GREEN);
        line(c, x, 113, x, 157, GREEN);
        line(c, x - 20, 135, x + 20, 135, GREEN);
        box(c, x - 25, 187, 50, 7, 0xa3bcb0);
        box(c, x - 18, 194, 36, 14, 0x5d7d70);
        candle(c, x - 36, 177, g->frame, GREEN);
        candle(c, x + 34, 177, g->frame + 5, GREEN);
        for (int i = 0; i < 8; i++)
            pixel(c, x - 25 + (i * 13) % 50, 120 + (i * 17 + g->frame / 3) % 62, GREEN);
    }
}
static const char *hunter[] = {
    "0000011111100000", "0000122222210000", "0001222222221000", "0001223333321000",
    "0000133331310000", "0000133333310000", "0000013333100000", "0000117777110000",
    "0001666444461000", "0016666444446100", "0016666444443610", "0166666555543310",
    "0166661555513310", "0166611555511110", "0166611555510000", "0166111777710000",
    "0166115555510000", "0166115555510000", "0166115555510000", "0166115515510000",
    "0161105515510000", "0111005515510000", "0000005515510000", "0000005515510000",
    "0000005515510000", "0000008818810000", "0000008818810000", "0000008818810000",
    "0000018818881000", "0000011111111000"};
static void player(Canvas c, const Game *g) {
    const Player *p = &g->player;
    if (p->hurt && (g->frame / 3) % 2)
        return;
    int x = (int)p->x - g->camera, y = (int)p->y;
    uint32_t palette[] = {0,        0x10131d, 0xc6b68e, 0xe4bf9b, 0xc7c6cb,
                          0x405374, 0x8b334d, 0xc9a96a, 0x292a3e};
    int stride = p->grounded && fabsf(p->vx) > 0.2f ? (g->frame / 6) % 4 : 0;
    for (int row = 0; row < 30; row++)
        for (int col = 0; col < 16; col++) {
            int color = hunter[row][col] - '0';
            if (!color)
                continue;
            int move = row >= 21 ? (col < 9 ? (stride == 1   ? -2
                                               : stride == 3 ? 2
                                                             : 0)
                                            : (stride == 1   ? 2
                                               : stride == 3 ? -2
                                                             : 0))
                                 : 0;
            pixel(c, x + (p->facing > 0 ? col : 15 - col) + move, y + row, palette[color]);
        }
    if (p->attack <= 15 && p->attack >= 5) {
        int sx = x + (p->facing > 0 ? 13 : 2), sy = y + 14;
        line(c, sx, sy, sx + p->facing * 32, sy - 3, PAPER);
        line(c, sx, sy + 1, sx + p->facing * 30, sy - 2, 0xa4bacb);
        line(c, sx + p->facing * 5, sy - 5, sx + p->facing * 5, sy + 5, GOLD);
        for (int i = -14; i < 15; i++) {
            int reach = 30 - (i * i) / 22;
            pixel(c, sx + p->facing * reach, sy + i, 0xeee4be);
        }
    }
}
static void enemy(Canvas c, const Game *g, const Enemy *e) {
    if (!e->alive)
        return;
    int x = (int)e->x - g->camera, y = (int)e->y;
    uint32_t bone = e->hurt ? PAPER : 0xbbc1bc, shadow = 0x626c76;
    if (e->type == BAT) {
        int flap = (e->phase / 7) % 2 ? 5 : -5;
        for (int i = 0; i < 12; i++) {
            line(c, x + 7, y + 6, x - i, y + flap + i / 3, 0x64445e);
            line(c, x + 9, y + 6, x + 18 + i, y + flap + i / 3, 0x64445e);
        }
        box(c, x + 5, y + 2, 8, 9, e->hurt ? PAPER : 0x9d6479);
        pixel(c, x + 6, y + 4, RED);
        pixel(c, x + 11, y + 4, RED);
        return;
    }
    int s = e->type == WARDEN ? 2 : 1, head = e->type == WARDEN ? 10 : 8;
    box(c, x + 5, y, head, head, bone);
    box(c, x + 7, y + 3, 2, 2, INK);
    box(c, x + head + 1, y + 3, 2, 2, RED);
    if (e->type == WARDEN) {
        box(c, x + 3, y + 11, 23, 20, e->phase % 180 > 120 ? 0x92435d : 0x615c78);
        border(c, x + 3, y + 11, 23, 20, GOLD);
        box(c, x + 12, y - 4, 3, 5, GOLD);
    } else {
        box(c, x + 8, y + 8, 3, 12, shadow);
        for (int i = 0; i < 3; i++)
            box(c, x + 4, y + 9 + i * 4, 12, 2, bone);
    }
    int hips = e->type == WARDEN ? 29 : 20;
    box(c, x + 4, y + hips, 15, 3, shadow);
    int walk = (e->phase / 13) % 2 ? 2 : -2;
    box(c, x + 4 + walk, y + hips + 3, 3 * s, 8, bone);
    box(c, x + 13 - walk, y + hips + 3, 3 * s, 8, bone);
    line(c, x + 3, y + 12, x - 1, y + 22, bone);
    line(c, x + 18, y + 12, x + 22, y + 20, bone);
    box(c, x + 22, y + 9, 2, 19, e->type == WARDEN ? RED : shadow);
    box(c, x + 19, y + 22, 8, 2, GOLD);
    if (e->hurt) {
        line(c, x - 4, y - 3, x + 1, y + 2, GOLD);
        line(c, x + 25, y - 3, x + 20, y + 2, PAPER);
    }
}
static void hud(Canvas c, const Game *g) {
    box(c, 0, 0, 400, 25, INK);
    text(c, 9, 8, "HP", 1, PAPER);
    for (int i = 0; i < MAX_HP; i++) {
        box(c, 28 + i * 12, 7, 9, 11, 0x452a3a);
        if (i < g->player.hp) {
            box(c, 29 + i * 12, 8, 7, 9, RED);
            box(c, 30 + i * 12, 8, 3, 2, 0xeea1a6);
        }
    }
    char s[64];
    snprintf(s, sizeof s, "GOLD %03u", g->gold);
    text(c, 316, 9, s, 1, GOLD);
    if (g->roomTitleTicks > 0) {
        box(c, 65, 30, 270, 18, INK);
        centered(c, 36, rooms[g->room].name, 1, PAPER);
    }
    if (g->room == 5 && g->enemies[0].alive) {
        box(c, 115, 217, 170, 15, INK);
        text(c, 121, 221, "WARDEN", 1, PAPER);
        box(c, 163, 221, 112, 6, 0x442438);
        box(c, 163, 221, g->enemies[0].hp * 112 / 12, 6, RED);
    }
    const Room *r = &rooms[g->room];
    if (r->saveRoom && fabsf(g->player.x - 210) < 45) {
        box(c, 98, 158, 228, 18, INK);
        text(c, 110, 164, "UP: SAVE AND REST", 1, GREEN);
    } else if (r->portalTarget >= 0 && fabsf(g->player.x - r->portalX) < 30) {
        const char *label = r->saveRoom    ? "UP: RETURN TO NAVE"
                            : g->room == 1 ? "UP: ENTER SANCTUARY"
                            : g->room == 3 ? "UP: ENTER BELL TOWER"
                                           : "UP: RETURN TO RAMPARTS";
        box(c, 85, 153, 235, 18, INK);
        text(c, 98, 159, label, 1, GOLD);
    }
    if (g->noticeTicks > 0) {
        box(c, 0, 233, 400, 7, INK);
        centered(c, 233, g->notice, 1, GREEN);
    }
}
static void scene(Canvas c, const Game *g) {
    roomBackground(c, g);
    for (int i = 0; i < rooms[g->room].enemyCount; i++)
        enemy(c, g, &g->enemies[i]);
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (g->pickups[i].life) {
            const Pickup *p = &g->pickups[i];
            int x = (int)p->x - g->camera, y = (int)p->y;
            disk(c, x, y, 3, p->heart ? RED : GOLD);
            pixel(c, x - 1, y - 1, PAPER);
        }
    player(c, g);
    hud(c, g);
}
static void map(Canvas c, const Game *g) {
    static const int connections[][2] = {{0, 1}, {1, 2}, {1, 3}, {3, 4}, {3, 5}};
    static const char *names[] = {"GATE", "NAVE", "SAVE", "WALL", "BELL", "CRYPT"};
    for (int i = 0; i < 5; i++) {
        int a = connections[i][0], b = connections[i][1];
        if ((g->visited & (1u << a)) && (g->visited & (1u << b)))
            line(c, 47 + rooms[a].mapX * 73, 56 + rooms[a].mapY * 42, 47 + rooms[b].mapX * 73,
                 56 + rooms[b].mapY * 42, 0x70657b);
    }
    for (int i = 0; i < ROOM_COUNT; i++) {
        const Room *r = &rooms[i];
        int x = 16 + r->mapX * 73, y = 40 + r->mapY * 42;
        if (!(g->visited & (1u << i)))
            continue;
        box(c, x, y, 63, 32, i == g->room ? 0x494133 : 0x242a3f);
        border(c, x, y, 63, 32, r->saveRoom ? GREEN : i == g->room ? GOLD : 0x60677c);
        text(c, x + 9, y + 13, names[i], 1, r->saveRoom ? GREEN : PAPER);
        if (i == g->room)
            box(c, x + 28, y + 25, 7, 3, (g->frame / 25) % 2 ? GOLD : PAPER);
    }
}
static void choice(Canvas c, int y, const char *label, bool selected) {
    box(c, 22, y, 276, 28, selected ? 0x323348 : 0x151a2b);
    if (selected)
        border(c, 22, y, 276, 28, GOLD);
    text(c, selected ? 37 : 43, y + 10, label, 1, selected ? PAPER : MUTED);
    if (selected)
        text(c, 27, y + 10, ">", 1, GOLD);
}
static void gameBottom(Canvas c, const Game *g) {
    box(c, 0, 0, 320, 240, INK);
    text(c, 14, 13, g->name, 2, PAPER);
    text(c, 230, 18, "CASTLE MAP", 1, GOLD);
    map(c, g);
    text(c, 17, 174, "GOLD", 1, GOLD);
    char s[64];
    snprintf(s, sizeof s, "%u   KILLS %u", g->gold, g->kills);
    text(c, 49, 174, s, 1, PAPER);
    text(c, 17, 188,
         g->warden ? "WARDEN SLAIN - SAVE YOUR PROGRESS" : "FIND THE SANCTUARY. DEFEAT THE WARDEN.",
         1, MUTED);
    box(c, 14, 207, 292, 23, 0x25283b);
    centered(c, 215, "START: MENU   SELECT: MAP", 1, PAPER);
}
static void titleTop(Canvas c, const Game *g) {
    backdrop(c, g->frame);
    centered(c, 60, "NIGHTFALL", 4, 0x282333);
    centered(c, 57, "NIGHTFALL", 4, PAPER);
    line(c, 91, 99, 309, 99, GOLD);
    centered(c, 111, "ASHEN KEEP", 2, GOLD);
    centered(c, 166, "A CASTLE HAS WOKEN.", 1, PAPER);
    centered(c, 182, "ENTER BEFORE THE LAST LIGHT DIES.", 1, MUTED);
    centered(c, 222, "PROTOTYPE 1.2.0", 1, MUTED);
}
void renderGame(const Game *g, Canvas top, Canvas bottom) {
    box(bottom, 0, 0, 320, 240, INK);
    if (g->screen == TITLE || g->screen == SLOTS || g->screen == NAME_ENTRY ||
        (g->screen == CONTROLS && g->returnScreen == TITLE))
        titleTop(top, g);
    else
        scene(top, g);
    char s[96];
    switch (g->screen) {
    case TITLE:
        centered(bottom, 26, "ASHEN KEEP", 2, GOLD);
        choice(bottom, 68, "ENTER THE CASTLE", g->selection == 0);
        choice(bottom, 104, "CONTROLS", g->selection == 1);
        choice(bottom, 140, "EXIT TO HOME", g->selection == 2);
        centered(bottom, 193, "D-PAD + A, OR TOUCH TO CHOOSE", 1, MUTED);
        centered(bottom, 212, "ORIGINAL 3DS HOMEBREW", 1, 0x62687f);
        break;
    case SLOTS:
        centered(bottom, 16, "CHOOSE YOUR SAVE", 2, PAPER);
        for (int i = 0; i < SAVE_SLOTS; i++) {
            int y = 48 + i * 50;
            panel(bottom, 18, y, 284, 44);
            if (i == g->selection)
                border(bottom, 18, y, 284, 44, GOLD);
            snprintf(s, sizeof s, "%d  %s", i + 1,
                     g->slotUsed[i] ? g->slots[i].name : "EMPTY SLOT");
            text(bottom, 28, y + 9, s, 1, g->slotUsed[i] ? PAPER : GOLD);
            if (g->slotUsed[i]) {
                snprintf(s, sizeof s, "%s   %u:%02u",
                         g->slots[i].checkpoint == 2 ? "SANCTUARY" : "GATEHOUSE",
                         (unsigned)(g->slots[i].seconds / 60),
                         (unsigned)(g->slots[i].seconds % 60));
                text(bottom, 46, y + 27, s, 1, MUTED);
            } else
                text(bottom, 46, y + 27, "BEGIN A NEW JOURNEY", 1, MUTED);
        }
        centered(bottom, 212, "A: SELECT   B: BACK", 1, MUTED);
        break;
    case NAME_ENTRY:
        centered(bottom, 90, "NAME YOUR HUNTER", 2, GOLD);
        centered(bottom, 124, "OPENING THE KEYBOARD...", 1, MUTED);
        break;
    case PLAY:
        gameBottom(bottom, g);
        break;
    case MAP:
        gameBottom(bottom, g);
        box(bottom, 0, 0, 320, 37, INK);
        centered(bottom, 12, "EXPLORED CASTLE", 2, GOLD);
        box(bottom, 0, 166, 320, 74, INK);
        centered(bottom, 178, "GOLD: YOU   GREEN: SAVE ROOM", 1, PAPER);
        centered(bottom, 197, rooms[g->room].name, 1, MUTED);
        centered(bottom, 220, "B / SELECT / TOUCH: CLOSE", 1, GOLD);
        panel(top, 107, 88, 186, 53);
        centered(top, 109, "MAP - PAUSED", 2, PAPER);
        break;
    case PAUSE:
        centered(bottom, 17, "PAUSED", 2, GOLD);
        snprintf(s, sizeof s, "%s   HP %d/%d", g->name, g->player.hp, MAX_HP);
        centered(bottom, 43, s, 1, PAPER);
        choice(bottom, 62, "RESUME", g->selection == 0);
        choice(bottom, 94, "CASTLE MAP", g->selection == 1);
        choice(bottom, 126, "CONTROLS", g->selection == 2);
        choice(bottom, 158, "RETURN TO TITLE", g->selection == 3);
        centered(bottom, 209, "SAVE AT THE SANCTUARY ALTAR", 1, GREEN);
        break;
    case CONTROLS:
        centered(bottom, 15, "CONTROLS", 2, GOLD);
        text(bottom, 22, 50, "D-PAD / CIRCLE PAD   MOVE", 1, PAPER);
        text(bottom, 22, 73, "B OR A              JUMP", 1, PAPER);
        text(bottom, 22, 96, "Y OR X              SWORD", 1, PAPER);
        text(bottom, 22, 119, "UP                  DOOR / SAVE", 1, PAPER);
        text(bottom, 22, 142, "START               PAUSE", 1, PAPER);
        text(bottom, 22, 165, "SELECT              MAP", 1, PAPER);
        centered(bottom, 192, "HOLD JUMP TO JUMP HIGHER", 1, MUTED);
        centered(bottom, 219, "A / B / TOUCH: BACK", 1, GOLD);
        break;
    case GAME_OVER:
        panel(top, 50, 77, 300, 77);
        centered(top, 94, "THE LIGHT FADES", 2, PAPER);
        centered(top, 129, "YOUR LAST SAVE REMAINS", 1, GOLD);
        centered(bottom, 53, "FALLEN HUNTER", 2, RED);
        centered(bottom, 100, "A: RETRY FROM LAST SAVE", 1, PAPER);
        centered(bottom, 129, "B: RETURN TO TITLE", 1, PAPER);
        centered(bottom, 190, "SAVE ROOMS RESTORE YOUR HEALTH", 1, MUTED);
        break;
    case QUIT_CONFIRM:
        centered(bottom, 39, "RETURN TO TITLE?", 2, GOLD);
        centered(bottom, 75, "PROGRESS SINCE YOUR SAVE WILL BE LOST.", 1, MUTED);
        choice(bottom, 110, "KEEP PLAYING", g->selection == 0);
        choice(bottom, 146, "LEAVE WITHOUT SAVING", g->selection == 1);
        break;
    }
    if (g->noticeTicks && g->screen == SLOTS)
        centered(bottom, 230, g->notice, 1, RED);
}
