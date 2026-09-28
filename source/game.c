#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int clampInt(int n, int low, int high) {
    return n < low ? low : n > high ? high : n;
}
bool overlaps(Rect a, Rect b) {
    return a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y;
}
Rect playerRect(const Player *p) {
    return (Rect){p->x, p->y, PLAYER_W, PLAYER_H};
}
Rect attackRect(const Player *p) {
    return (Rect){p->facing > 0 ? p->x + 10 : p->x - 32, p->y + 6, 38, 23};
}
void gameNotice(Game *g, const char *message) {
    snprintf(g->notice, sizeof g->notice, "%s", message);
    g->noticeTicks = 180;
}
static void refreshSlots(Game *g) {
    for (int i = 0; i < SAVE_SLOTS; i++)
        g->slotUsed[i] = saveRead(i, &g->slots[i]);
}
void gameInit(Game *g) {
    memset(g, 0, sizeof *g);
    g->screen = TITLE;
    refreshSlots(g);
}
void gameEnterRoom(Game *g, int room, float x) {
    if (room < 0 || room >= ROOM_COUNT)
        return;
    g->room = room;
    g->visited |= 1u << room;
    g->roomTitleTicks = 150;
    g->portalCooldown = 35;
    g->player.x = x;
    g->player.y = FLOOR_Y - PLAYER_H;
    g->player.vx = g->player.vy = 0;
    g->player.grounded = true;
    g->player.attack = g->player.cooldown = 0;
    g->player.hurt = 45;
    g->player.hitMask = 0;
    memset(g->enemies, 0, sizeof g->enemies);
    memset(g->pickups, 0, sizeof g->pickups);
    const Room *r = &rooms[room];
    for (int i = 0; i < r->enemyCount; i++) {
        const Spawn *s = &r->spawns[i];
        Enemy *e = &g->enemies[i];
        *e = (Enemy){s->type, s->x,     s->y,   s->y,
                     s->left, s->right, -0.55f, s->type == WARDEN ? 12 : s->type == BAT ? 2 : 3,
                     0,       i * 71,   true};
        if (e->type == WARDEN && g->warden)
            e->alive = false;
    }
    g->camera = clampInt((int)x - 190, 0, r->width - 400);
}
static SaveRecord recordFromGame(const Game *g) {
    SaveRecord r = {0};
    snprintf(r.name, sizeof r.name, "%s", g->name);
    r.visited = g->visited;
    r.gold = g->gold;
    r.kills = g->kills;
    r.seconds = g->seconds;
    r.checkpoint = g->room;
    r.warden = g->warden;
    return r;
}
bool gameSave(Game *g) {
    if (!rooms[g->room].saveRoom)
        return false;
    SaveRecord r = recordFromGame(g);
    if (!saveWrite(g->slot, &r)) {
        gameNotice(g, "SAVE FAILED - check SD card");
        return false;
    }
    g->slots[g->slot] = r;
    g->slotUsed[g->slot] = true;
    g->player.hp = MAX_HP;
    gameNotice(g, "Progress saved. Health restored.");
    return true;
}
void gameNew(Game *g, const char *name) {
    char chosenName[16];
    snprintf(chosenName, sizeof chosenName, "%.*s", NAME_LENGTH, name[0] ? name : "Hunter");
    memset(&g->player, 0, sizeof g->player);
    g->player.hp = MAX_HP;
    g->player.facing = 1;
    g->gold = g->kills = g->seconds = g->playTicks = g->visited = 0;
    g->warden = false;
    snprintf(g->name, sizeof g->name, "%s", chosenName);
    gameEnterRoom(g, 0, 56);
    g->screen = PLAY;
    SaveRecord r = recordFromGame(g);
    if (saveWrite(g->slot, &r)) {
        g->slots[g->slot] = r;
        g->slotUsed[g->slot] = true;
        gameNotice(g, "Find the sanctuary to save your journey.");
    } else
        gameNotice(g, "SAVE FAILED - playing without a save");
}
void gameLoad(Game *g, int slot) {
    SaveRecord r;
    if (!saveRead(slot, &r)) {
        gameNotice(g, "Unable to read this save.");
        return;
    }
    g->slot = slot;
    g->slots[slot] = r;
    g->slotUsed[slot] = true;
    snprintf(g->name, sizeof g->name, "%s", r.name);
    g->visited = r.visited;
    g->gold = r.gold;
    g->kills = r.kills;
    g->seconds = r.seconds;
    g->warden = r.warden;
    memset(&g->player, 0, sizeof g->player);
    g->player.hp = MAX_HP;
    g->player.facing = 1;
    g->playTicks = 0;
    gameEnterRoom(g, r.checkpoint, r.checkpoint == 2 ? 280 : 56);
    g->screen = PLAY;
}
static void movePlayer(Game *g, Input in) {
    Player *p = &g->player;
    const Room *r = &rooms[g->room];
    if (p->hurt > 0)
        p->hurt--;
    if (p->attack > 0)
        p->attack--;
    if (p->cooldown > 0)
        p->cooldown--;
    if (p->grounded)
        p->coyote = 6;
    else if (p->coyote > 0)
        p->coyote--;
    if (in.pressed & BTN_JUMP)
        p->jumpBuffer = 7;
    else if (p->jumpBuffer > 0)
        p->jumpBuffer--;
    int direction = ((in.held & BTN_RIGHT) != 0) - ((in.held & BTN_LEFT) != 0);
    if (p->hurt < 45) {
        p->vx = direction * (p->attack ? 1.35f : 2.0f);
        if (direction && !p->attack)
            p->facing = direction;
    } else
        p->vx *= 0.92f;
    if (p->jumpBuffer && p->coyote) {
        p->vy = -6.8f;
        p->grounded = false;
        p->coyote = p->jumpBuffer = 0;
    }
    if ((in.released & BTN_JUMP) && p->vy < -3.0f)
        p->vy = -3.0f;
    if ((in.pressed & BTN_ATTACK) && !p->cooldown) {
        p->attack = 19;
        p->cooldown = 26;
        p->hitMask = 0;
    }
    float oldBottom = p->y + PLAYER_H;
    p->x += p->vx;
    p->vy = fminf(p->vy + 0.32f, 7.0f);
    p->y += p->vy;
    p->grounded = false;
    if (p->vy >= 0) {
        float landing = FLOOR_Y;
        for (int i = 0; i < r->platformCount; i++) {
            Platform s = r->platforms[i];
            if (p->x + PLAYER_W > s.x && p->x < s.x + s.w && oldBottom <= s.y + 0.1f &&
                p->y + PLAYER_H >= s.y && s.y < landing)
                landing = s.y;
        }
        if (p->y + PLAYER_H >= landing) {
            p->y = landing - PLAYER_H;
            p->vy = 0;
            p->grounded = true;
        }
    }
    if (p->x < 0) {
        if (r->leftRoom >= 0)
            gameEnterRoom(g, r->leftRoom, rooms[r->leftRoom].width - 30);
        else
            p->x = 0;
    } else if (p->x > r->width - PLAYER_W) {
        if (r->rightRoom >= 0)
            gameEnterRoom(g, r->rightRoom, 16);
        else
            p->x = r->width - PLAYER_W;
    }
    if (p->y < 22) {
        p->y = 22;
        p->vy = 0;
    }
}
static void dropPickup(Game *g, Enemy *e) {
    for (int i = 0; i < MAX_PICKUPS; i++)
        if (!g->pickups[i].life) {
            g->pickups[i] = (Pickup){e->x, e->y + 12, 900, (g->kills % 3) == 0};
            break;
        }
}
static void hurtPlayer(Game *g, Enemy *e) {
    Player *p = &g->player;
    if (p->hurt)
        return;
    p->hp -= e->type == WARDEN ? 2 : 1;
    p->hurt = 65;
    p->vx = p->x < e->x ? -3.0f : 3.0f;
    p->vy = -3.5f;
    if (p->hp <= 0) {
        p->hp = 0;
        g->screen = GAME_OVER;
        g->selection = 0;
    }
}
static void updateEnemies(Game *g) {
    Player *p = &g->player;
    const Room *r = &rooms[g->room];
    for (int i = 0; i < r->enemyCount; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->alive)
            continue;
        e->phase++;
        if (e->hurt)
            e->hurt--;
        if (!e->hurt) {
            if (e->type == BAT) {
                e->x += e->vx * 1.4f;
                e->y = e->baseY + sinf(e->phase * 0.045f) * 22;
                if (fabsf(p->x - e->x) < 100)
                    e->y += (p->y - e->baseY) * 0.35f;
            } else {
                float speed = e->type == WARDEN && e->phase % 180 > 120 ? 2.1f : 0.55f;
                if (e->type == WARDEN && e->phase % 180 == 120)
                    e->vx = p->x < e->x ? -0.55f : 0.55f;
                e->x += e->vx < 0 ? -speed : speed;
            }
            if (e->x < e->left) {
                e->x = e->left;
                e->vx = fabsf(e->vx);
            }
            if (e->x > e->right) {
                e->x = e->right;
                e->vx = -fabsf(e->vx);
            }
        }
        Rect body = {e->x, e->y, e->type == WARDEN ? 25 : 18,
                     e->type == BAT      ? 13
                     : e->type == WARDEN ? 42
                                         : 30};
        if (p->attack <= 15 && p->attack >= 5 && !(p->hitMask & (1u << i)) &&
            overlaps(attackRect(p), body)) {
            p->hitMask |= 1u << i;
            e->hp--;
            e->hurt = 14;
            e->x += p->facing * 10;
            if (e->hp <= 0) {
                e->alive = false;
                g->kills++;
                dropPickup(g, e);
                if (e->type == WARDEN) {
                    g->warden = true;
                    g->gold += 25;
                    gameNotice(g, "Warden defeated! Return to the sanctuary.");
                }
            }
        }
        if (e->alive && overlaps(playerRect(p), body))
            hurtPlayer(g, e);
    }
}
static void updatePickups(Game *g) {
    for (int i = 0; i < MAX_PICKUPS; i++) {
        Pickup *p = &g->pickups[i];
        if (!p->life)
            continue;
        p->life--;
        if (p->y < 196)
            p->y += 1.5f;
        if (overlaps(playerRect(&g->player), (Rect){p->x - 4, p->y - 4, 12, 12})) {
            if (p->heart)
                g->player.hp = clampInt(g->player.hp + 2, 0, MAX_HP);
            else
                g->gold++;
            p->life = 0;
        }
    }
}
static void playTick(Game *g, Input in) {
    if (in.pressed & BTN_PAUSE) {
        g->screen = PAUSE;
        g->selection = 0;
        return;
    }
    if (in.pressed & BTN_MAP || (in.touched && in.touchY < 168)) {
        g->screen = MAP;
        return;
    }
    if (in.touched && in.touchY >= 192) {
        g->screen = PAUSE;
        g->selection = 0;
        return;
    }
    if (++g->playTicks >= 60) {
        g->seconds++;
        g->playTicks = 0;
    }
    if (g->portalCooldown)
        g->portalCooldown--;
    movePlayer(g, in);
    updateEnemies(g);
    if (g->screen != PLAY)
        return;
    updatePickups(g);
    const Room *r = &rooms[g->room];
    Player *p = &g->player;
    if ((in.pressed & BTN_UP) && !g->portalCooldown) {
        if (r->saveRoom && fabsf(p->x - 210) < 45) {
            gameSave(g);
            g->portalCooldown = 30;
        } else if (r->portalTarget >= 0 && fabsf(p->x - r->portalX) < 30 && p->grounded)
            gameEnterRoom(g, r->portalTarget, r->portalArrival);
    }
    g->camera = clampInt((int)p->x - 190, 0, rooms[g->room].width - 400);
}
static bool touchRow(Game *g, Input in, int start, int height, int count) {
    if (!in.touched || in.touchX < 18 || in.touchX > 302 || in.touchY < start ||
        in.touchY >= start + height * count)
        return false;
    g->selection = (in.touchY - start) / height;
    return true;
}
static void menuTick(Game *g, Input in) {
    int count = g->screen == TITLE ? 3 : g->screen == SLOTS ? 3 : g->screen == PAUSE ? 4 : 2;
    if (in.pressed & BTN_UP)
        g->selection = (g->selection + count - 1) % count;
    if (in.pressed & BTN_DOWN)
        g->selection = (g->selection + 1) % count;
    bool confirm = (in.pressed & BTN_CONFIRM) != 0;
    if (g->screen == SLOTS)
        confirm |= touchRow(g, in, 48, 50, 3);
    if (g->screen == TITLE)
        confirm |= touchRow(g, in, 68, 36, 3);
    if (g->screen == PAUSE)
        confirm |= touchRow(g, in, 62, 32, 4);
    if (g->screen == QUIT_CONFIRM)
        confirm |= touchRow(g, in, 110, 36, 2);
    if (g->screen == TITLE && confirm) {
        if (g->selection == 0) {
            refreshSlots(g);
            g->screen = SLOTS;
            g->selection = 0;
        } else if (g->selection == 1) {
            g->returnScreen = TITLE;
            g->screen = CONTROLS;
        } else
            g->exitRequested = true;
    } else if (g->screen == SLOTS) {
        if (in.pressed & BTN_CANCEL)
            g->screen = TITLE;
        else if (confirm) {
            g->slot = g->selection;
            if (g->slotUsed[g->slot])
                gameLoad(g, g->slot);
            else
                g->screen = NAME_ENTRY;
        }
    } else if (g->screen == PAUSE) {
        if (in.pressed & (BTN_CANCEL | BTN_PAUSE))
            g->screen = PLAY;
        else if (confirm) {
            if (g->selection == 0)
                g->screen = PLAY;
            else if (g->selection == 1)
                g->screen = MAP;
            else if (g->selection == 2) {
                g->returnScreen = PAUSE;
                g->screen = CONTROLS;
            } else {
                g->screen = QUIT_CONFIRM;
                g->selection = 0;
            }
        }
    } else if (g->screen == QUIT_CONFIRM) {
        if (in.pressed & BTN_CANCEL)
            g->screen = PAUSE;
        else if (confirm) {
            g->screen = g->selection == 0 ? PAUSE : TITLE;
            g->selection = 0;
        }
    } else if (g->screen == GAME_OVER) {
        if (confirm) {
            if (g->slotUsed[g->slot])
                gameLoad(g, g->slot);
            else
                gameNew(g, g->name);
        } else if (in.pressed & BTN_CANCEL) {
            g->screen = TITLE;
            g->selection = 0;
        }
    }
}
void gameTick(Game *g, Input in) {
    g->frame++;
    if (g->noticeTicks)
        g->noticeTicks--;
    if (g->screen == PLAY) {
        if (g->roomTitleTicks)
            g->roomTitleTicks--;
        playTick(g, in);
    } else if (g->screen == MAP) {
        if (in.pressed & (BTN_MAP | BTN_PAUSE | BTN_CANCEL | BTN_CONFIRM) || in.touched)
            g->screen = PLAY;
    } else if (g->screen == CONTROLS) {
        if (in.pressed & (BTN_CANCEL | BTN_CONFIRM | BTN_PAUSE) || in.touched)
            g->screen = g->returnScreen;
    } else if (g->screen != NAME_ENTRY)
        menuTick(g, in);
}
