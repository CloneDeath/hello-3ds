#include "render.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "art.inc"
static float eyeOffset;

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
// Positive depth is behind the display. Actors and collision surfaces use depth zero.
static int depth(float z) { return (int)lroundf(eyeOffset * z); }
static void art(Canvas c, const Art *a, int x, int y, bool flip, int light) {
    int x0 = x < 0 ? -x : 0, y0 = y < 0 ? -y : 0;
    int x1 = a->w < c.width-x ? a->w : c.width-x;
    int y1 = a->h < c.height-y ? a->h : c.height-y;
    for (int yy=y0; yy<y1; yy++) for (int xx=x0; xx<x1; xx++) {
        uint16_t v=a->pixels[yy*a->w+(flip ? a->w-1-xx : xx)];
        if (!v) continue;
        unsigned r=((v>>11)*255/31)*light/255;
        unsigned g=(((v>>5)&63)*255/63)*light/255;
        unsigned b=((v&31)*255/31)*light/255;
        c.pixels[(y+yy)*c.width+x+xx]=(r<<16)|(g<<8)|b;
    }
}
static void roomBackground(Canvas c, const Game *g) {
    const Room *r = &rooms[g->room];
    box(c,0,0,400,240,0x181927);
    bool outside = g->room == 0 || g->room == 3 || g->room == 4;
    if (outside) {
        for (int x=-256-(int)(g->camera*.18f)%256; x<420; x+=256)
            art(c,&art_landscape,x+depth(6),-62,false,115);
        disk(c,324-g->camera/12+depth(7),55,19,0xaba9c1);
        disk(c,330-g->camera/12+depth(7),49,17,0x1b2338);
    } else {
        for (int y=25;y<208;y+=48)
            for (int x=-32-(int)(g->camera*.45f)%32;x<420;x+=32)
                art(c,&art_brick,x+depth(4),y,false,105);
    }
    for (int x=-240-(int)(g->camera*.55f)%240;x<480;x+=240) {
        if (g->room != 3)
            art(c,&art_window,x+35+depth(3.5f),17,false,205);
        art(c,&art_column,x-40+depth(2),17,false,220);
    }
    if (g->room == 5)
        art(c,&art_crest,300-g->camera+depth(2),16,false,255);
    // Receding top faces sit behind the collision edge; front faces remain at zero depth.
    for (int y=0;y<7;y++)
        box(c,0,201+y,400,1,0x3b3a52+(y<<16)+(y<<8));
    for (int x=-(int)g->camera%48-48;x<400;x+=48)
        art(c,&art_floor,x,208,false,235);
    for (int i=0;i<r->platformCount;i++) {
        Platform p=r->platforms[i];
        int x=p.x-g->camera;
        for (int y=0;y<6;y++)
            box(c,x+depth((6-y)*.25f),p.y-6+y,p.w,1,0x4b485e);
        for(int xx=0;xx<p.w;xx+=16) {
            box(c,x+xx,p.y,p.w-xx<16?p.w-xx:16,10,0x343345);
            line(c,x+xx,p.y+1,x+xx+(p.w-xx<16?p.w-xx:16)-1,p.y+1,0x9490a4);
            line(c,x+xx,p.y+9,x+xx+10,p.y+9,0x181923);
        }
    }
    if (r->leftRoom < 0) {
        box(c, -g->camera, 110, 11, 98, r->stone);
    }
    if (r->rightRoom < 0)
        box(c, r->width - 11 - g->camera, 110, 11, 98, r->stone);
    if (r->portalX >= 0) {
        int x = r->portalX - g->camera;
        art(c, &art_door, x - 8, 144, false, 255);
        box(c, x - 14, 155, 43, 53, 0x080e19);
        border(c, x - 17, 152, 49, 56, r->accent);
        for (int i = 0; i < 4; i++)
            box(c, x - 10 + i * 9, 158, 2, 48, 0x393548);
        candle(c, x - 26, 179, g->frame, GOLD);
        candle(c, x + 38, 179, g->frame, GOLD);
    }
    if (r->saveRoom) {
        int x = 218 - g->camera;
        art(c, &art_altar, x - 64 + depth(1), 17, false, 255);
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
static void player(Canvas c, const Game *g) {
    const Player *p=&g->player;
    if (p->hurt && (g->frame/3)%2) return;
    static const Art *walk[]={&art_knight_walk0,&art_knight_walk1,&art_knight_walk2,
        &art_knight_walk3,&art_knight_walk4,&art_knight_walk5,&art_knight_walk6,&art_knight_walk7};
    static const Art *attack[]={&art_knight_attack0,&art_knight_attack1,&art_knight_attack2};
    const Art *a=&art_knight_idle;
    if (!p->grounded) a=&art_knight_jump;
    else if (fabsf(p->vx)>.2f) a=walk[(g->frame/6)%8];
    if (p->attack) a=attack[(19-p->attack)*3/19];
    int x=(int)p->x-g->camera+8;
    // All source frames share a stable foot baseline; wider sword frames share the same body pivot.
    int pivot=p->attack?43:27;
    art(c,a,x-(p->facing<0?a->w-1-pivot:pivot),(int)p->y+30-a->h+4,p->facing<0,255);
}
static void enemy(Canvas c, const Game *g, const Enemy *e) {
    if (!e->alive || (e->hurt && (g->frame/2)%2)) return;
    static const Art *bats[]={&art_bat0,&art_bat1,&art_bat2,&art_bat3};
    static const Art *ghouls[]={&art_ghoul0,&art_ghoul1,&art_ghoul2,&art_ghoul3,
        &art_ghoul4,&art_ghoul5,&art_ghoul6};
    static const Art *wizards[]={&art_wizard0,&art_wizard1,&art_wizard2,&art_wizard3,&art_wizard4};
    const Art *a=e->type==BAT?bats[(e->phase/7)%4]:e->type==WARDEN?wizards[(e->phase/8)%5]:ghouls[(e->phase/7)%7];
    int feet=(int)e->y+(e->type==BAT?23:e->type==WARDEN?49:36);
    art(c,a,(int)e->x-g->camera+9-a->w/2,feet-a->h,e->x>g->player.x,255);
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
    // Near pillars move faster than the world and project slightly in front of the screen.
    for (int x=-480-(int)(g->camera*1.12f)%480;x<440;x+=480)
        art(c,&art_foreground,x-40+depth(-1.5f),70,false,90);
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
    art(c,&art_column,-48+depth(2),30,false,220);
    art(c,&art_column,320+depth(2),30,false,220);
    centered(c, 60, "NIGHTFALL", 4, 0x282333);
    centered(c, 57, "NIGHTFALL", 4, PAPER);
    line(c, 91, 99, 309, 99, GOLD);
    centered(c, 111, "ASHEN KEEP", 2, GOLD);
    centered(c, 166, "A CASTLE HAS WOKEN.", 1, PAPER);
    centered(c, 182, "ENTER BEFORE THE LAST LIGHT DIES.", 1, MUTED);
    centered(c, 222, "PROTOTYPE 1.3.0", 1, MUTED);
}
void renderGameEye(const Game *g, Canvas top, Canvas bottom, float eye) {
    eyeOffset = fmaxf(-1, fminf(1, eye));
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

void renderGame(const Game *g, Canvas top, Canvas bottom) {
    renderGameEye(g,top,bottom,0);
}
