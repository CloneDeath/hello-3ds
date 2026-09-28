#ifndef NIGHTFALL_GAME_H
#define NIGHTFALL_GAME_H
#include <stdbool.h>
#include <stdint.h>

#define ROOM_COUNT 6
#define MAX_ENEMIES 8
#define MAX_PLATFORMS 7
#define MAX_PICKUPS 12
#define MAX_HP 8
#define PLAYER_W 16
#define PLAYER_H 30
#define FLOOR_Y 208
#define SAVE_SLOTS 3
#define NAME_LENGTH 12

enum Button {
    BTN_LEFT = 1,
    BTN_RIGHT = 2,
    BTN_UP = 4,
    BTN_DOWN = 8,
    BTN_JUMP = 16,
    BTN_ATTACK = 32,
    BTN_CONFIRM = 64,
    BTN_CANCEL = 128,
    BTN_PAUSE = 256,
    BTN_MAP = 512
};
typedef struct {
    unsigned held, pressed, released;
    bool touched;
    int touchX, touchY;
} Input;
typedef enum {
    TITLE,
    SLOTS,
    NAME_ENTRY,
    PLAY,
    PAUSE,
    MAP,
    CONTROLS,
    GAME_OVER,
    QUIT_CONFIRM
} Screen;
typedef struct {
    float x, y, w, h;
} Rect;
typedef struct {
    int x, y, w;
} Platform;
typedef enum { SKELETON, BAT, WARDEN } EnemyType;
typedef struct {
    EnemyType type;
    float x, y, left, right;
} Spawn;
typedef struct {
    const char *name, *subtitle;
    int width, mapX, mapY, leftRoom, rightRoom;
    uint32_t sky, stone, accent;
    Platform platforms[MAX_PLATFORMS];
    int platformCount;
    Spawn spawns[MAX_ENEMIES];
    int enemyCount;
    int portalX, portalTarget, portalArrival;
    bool saveRoom;
} Room;
typedef struct {
    float x, y, vx, vy;
    int facing, hp;
    bool grounded;
    int coyote, jumpBuffer, attack, cooldown, hurt;
    unsigned hitMask;
} Player;
typedef struct {
    EnemyType type;
    float x, y, baseY, left, right, vx;
    int hp, hurt, phase;
    bool alive;
} Enemy;
typedef struct {
    float x, y;
    int life;
    bool heart;
} Pickup;
typedef struct {
    uint32_t magic, version;
    char name[16];
    uint32_t visited, gold, kills, seconds, checkpoint, warden, checksum;
} SaveRecord;
typedef struct {
    Screen screen, returnScreen;
    Player player;
    Enemy enemies[MAX_ENEMIES];
    Pickup pickups[MAX_PICKUPS];
    int room, slot, selection, frame, playTicks, camera, noticeTicks, roomTitleTicks,
        portalCooldown;
    unsigned visited, gold, kills, seconds;
    bool warden, exitRequested;
    char name[16], notice[64];
    SaveRecord slots[SAVE_SLOTS];
    bool slotUsed[SAVE_SLOTS];
} Game;
extern const Room rooms[ROOM_COUNT];
bool overlaps(Rect a, Rect b);
Rect playerRect(const Player *p);
Rect attackRect(const Player *p);
void gameInit(Game *g);
void gameTick(Game *g, Input input);
void gameNew(Game *g, const char *name);
void gameLoad(Game *g, int slot);
void gameEnterRoom(Game *g, int room, float x);
bool gameSave(Game *g);
void gameNotice(Game *g, const char *message);
bool saveRead(int slot, SaveRecord *record);
bool saveWrite(int slot, SaveRecord *record);
void saveSetDirectory(const char *directory);
#endif
