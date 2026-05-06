/*
 * Dungeon & Dragons Terminal Roguelike
 * Built on top of the TUI framework by the Chinese programmer.
 * (The original framework was excellent — good dirty-cell double buffering,
 *  UTF-8 support, linker-section module init, and truecolor ANSI.)
 *
 * Press: WASD / hjkl / arrow keys to move
 *        '.' to wait a turn
 *        'i' to view inventory
 *        'p' to pick up item
 *        'q' to quit
 */

#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* =========================================================
 * 0. Basic Types
 * ========================================================= */

typedef struct { uint8_t r, g, b; } RGB;

typedef struct {
    uint32_t cp;
    RGB fg, bg;
    bool dirty;
} Cell;

typedef struct {
    int width, height;
    Cell *cells;
} Screen;

/* =========================================================
 * 1. Module System (Linker Section)
 * ========================================================= */

typedef int (*initcall_t)(void);
extern initcall_t __start_initcalls[];
extern initcall_t __stop_initcalls[];

#define APP_INIT(fn) \
    static initcall_t __init_##fn \
        __attribute__((used, section("initcalls"), aligned(sizeof(void *)))) = fn;

/* =========================================================
 * 2. Colors
 * ========================================================= */

#define C_BLACK      ((RGB){10,  10,  15})
#define C_FLOOR      ((RGB){30,  30,  45})
#define C_WALL       ((RGB){60,  55,  80})
#define C_WALL_FG    ((RGB){90,  85, 110})
#define C_PLAYER     ((RGB){220, 220, 100})
#define C_GOBLIN     ((RGB){80,  180,  80})
#define C_ORC        ((RGB){180,  80,  80})
#define C_TROLL      ((RGB){140,  90, 200})
#define C_ITEM_POTION ((RGB){220,  80, 180})
#define C_ITEM_SWORD  ((RGB){160, 220, 255})
#define C_ITEM_ARMOR  ((RGB){200, 180, 120})
#define C_ITEM_GOLD   ((RGB){255, 215,  50})
#define C_STAIRS      ((RGB){100, 200, 255})
#define C_UI_BG       ((RGB){18,  16,  28})
#define C_UI_BORDER   ((RGB){80,  60, 130})
#define C_UI_TEXT     ((RGB){200, 195, 220})
#define C_UI_TITLE    ((RGB){255, 200,  80})
#define C_HP_HIGH     ((RGB){80,  220, 100})
#define C_HP_MED      ((RGB){220, 180,  60})
#define C_HP_LOW      ((RGB){220,  60,  60})
#define C_MSG_GOOD    ((RGB){100, 220, 140})
#define C_MSG_BAD     ((RGB){220, 100, 100})
#define C_MSG_INFO    ((RGB){180, 180, 220})
#define C_MSG_GOLD    ((RGB){255, 200,  80})

/* =========================================================
 * 3. Game Constants
 * ========================================================= */

#define MAP_W       80
#define MAP_H       50
#define MAX_ROOMS   15
#define MIN_ROOM    5
#define MAX_ROOM    12
#define MAX_ENEMIES 30
#define MAX_ITEMS   40
#define MSG_COUNT   6
#define INV_SIZE    20
#define MAX_DEPTH   5

/* =========================================================
 * 4. Tile Types
 * ========================================================= */

typedef enum {
    TILE_NONE = 0,
    TILE_FLOOR,
    TILE_WALL,
    TILE_DOOR,
    TILE_STAIRS_DOWN,
} TileType;

typedef struct {
    TileType type;
    bool visible;
    bool explored;
} Tile;

/* =========================================================
 * 5. Items
 * ========================================================= */

typedef enum {
    ITEM_NONE = 0,
    ITEM_HEALTH_POTION,
    ITEM_STRENGTH_POTION,
    ITEM_SWORD,
    ITEM_SHIELD,
    ITEM_ARMOR,
    ITEM_GOLD,
} ItemType;

typedef struct {
    ItemType type;
    int x, y;
    bool picked;
    int value; // gold amount or stat value
} Item;

/* =========================================================
 * 6. Enemies
 * ========================================================= */

typedef enum {
    MOB_NONE = 0,
    MOB_GOBLIN,
    MOB_ORC,
    MOB_TROLL,
} MobType;

typedef struct {
    MobType type;
    int x, y;
    int hp, max_hp;
    int attack, defense;
    bool alive;
    int xp_value;
} Enemy;

/* =========================================================
 * 7. Player
 * ========================================================= */

typedef struct {
    int x, y;
    int hp, max_hp;
    int attack, defense;
    int level, xp, xp_next;
    int gold;
    ItemType inventory[INV_SIZE];
    int inv_count;
    int depth; // dungeon floor
} Player;

/* =========================================================
 * 8. Message Log
 * ========================================================= */

typedef struct {
    char text[80];
    RGB  color;
} Message;

/* =========================================================
 * 9. UI Mode
 * ========================================================= */

typedef enum {
    UI_GAME = 0,
    UI_INVENTORY,
    UI_GAME_OVER,
    UI_VICTORY,
} UIMode;

/* =========================================================
 * 10. Room
 * ========================================================= */

typedef struct {
    int x, y, w, h;
} Room;

/* =========================================================
 * 11. Global State
 * ========================================================= */

static struct {
    Screen *screen;
    bool is_running;
    volatile sig_atomic_t win_resized;

    Tile    map[MAP_H][MAP_W];
    Enemy   enemies[MAX_ENEMIES];
    Item    items[MAX_ITEMS];
    Player  player;
    Message messages[MSG_COUNT];
    int     msg_head; // ring buffer head
    UIMode  ui_mode;

    Room    rooms[MAX_ROOMS];
    int     room_count;
    int     enemy_count;
    int     item_count;
} G;

/* =========================================================
 * 12. UTF-8 Encode
 * ========================================================= */

static int utf8_encode(uint32_t cp, char *out)
{
    if (cp < 0x80)       { out[0]=(char)cp; return 1; }
    else if (cp < 0x800) { out[0]=(char)(0xC0|(cp>>6)); out[1]=(char)(0x80|(cp&0x3F)); return 2; }
    else if (cp < 0x10000) {
        out[0]=(char)(0xE0|(cp>>12)); out[1]=(char)(0x80|((cp>>6)&0x3F));
        out[2]=(char)(0x80|(cp&0x3F)); return 3;
    } else {
        out[0]=(char)(0xF0|(cp>>18)); out[1]=(char)(0x80|((cp>>12)&0x3F));
        out[2]=(char)(0x80|((cp>>6)&0x3F)); out[3]=(char)(0x80|(cp&0x3F)); return 4;
    }
}

/* =========================================================
 * 13. Terminal Control
 * ========================================================= */

static struct termios orig_termios;

static void handle_sigwinch(int sig) { (void)sig; G.win_resized = 1; }

static int terminal_restore(void)
{
    write(STDOUT_FILENO, "\x1b[?1049l", 8);
    write(STDOUT_FILENO, "\x1b[?25h",   6);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    return 0;
}

static int terminal_init(void)
{
    if (!isatty(STDIN_FILENO)) return -1;
    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0) return -1;

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT|ICRNL|INPCK|ISTRIP|IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |=  (CS8);
    raw.c_lflag &= ~(ECHO|ICANON|IEXTEN|ISIG);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) return -1;

    write(STDOUT_FILENO, "\x1b[?1049h", 8);
    write(STDOUT_FILENO, "\x1b[?25l",   6);
    signal(SIGWINCH, handle_sigwinch);
    return 0;
}
APP_INIT(terminal_init);

/* =========================================================
 * 14. Screen Buffer
 * ========================================================= */

static void screen_resize(int w, int h)
{
    if (G.screen) { free(G.screen->cells); free(G.screen); }
    G.screen = (Screen *)calloc(1, sizeof(Screen));
    G.screen->width  = w;
    G.screen->height = h;
    G.screen->cells  = (Cell *)calloc(w * h, sizeof(Cell));
    for (int i = 0; i < w * h; i++) {
        G.screen->cells[i].cp    = ' ';
        G.screen->cells[i].fg   = C_UI_TEXT;
        G.screen->cells[i].bg   = C_BLACK;
        G.screen->cells[i].dirty = true;
    }
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

static int screen_init(void)
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    screen_resize(ws.ws_col, ws.ws_row);
    return 0;
}
APP_INIT(screen_init);

static void set_cell(int x, int y, uint32_t cp, RGB fg, RGB bg)
{
    if (!G.screen) return;
    if (x < 0 || x >= G.screen->width || y < 0 || y >= G.screen->height) return;
    int   idx = y * G.screen->width + x;
    Cell *c   = &G.screen->cells[idx];
    if (c->cp != cp || memcmp(&c->fg, &fg, 3) || memcmp(&c->bg, &bg, 3)) {
        c->cp = cp; c->fg = fg; c->bg = bg; c->dirty = true;
    }
}

/* =========================================================
 * 15. Renderer
 * ========================================================= */

static void screen_flush(void)
{
    char buffer[8192];
    int  buf_pos = 0;
    int  cur_x = -1, cur_y = -1;
    RGB  last_fg = {0,0,0}, last_bg = {0,0,0};
    bool color_set = false;

    auto void buf_write(const char *d, int l) {
        if (buf_pos + l > (int)sizeof(buffer)) { write(STDOUT_FILENO, buffer, buf_pos); buf_pos = 0; }
        memcpy(buffer + buf_pos, d, l); buf_pos += l;
    }

    for (int y = 0; y < G.screen->height; y++) {
        for (int x = 0; x < G.screen->width; x++) {
            Cell *c = &G.screen->cells[y * G.screen->width + x];
            if (!c->dirty) continue;

            if (x != cur_x || y != cur_y) {
                char cmd[32];
                int  l = snprintf(cmd, sizeof(cmd), "\x1b[%d;%dH", y+1, x+1);
                buf_write(cmd, l);
                cur_x = x; cur_y = y;
            }

            if (!color_set || memcmp(&c->fg, &last_fg, 3) || memcmp(&c->bg, &last_bg, 3)) {
                char cmd[64];
                int  l = snprintf(cmd, sizeof(cmd),
                    "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm",
                    c->fg.r, c->fg.g, c->fg.b, c->bg.r, c->bg.g, c->bg.b);
                buf_write(cmd, l);
                last_fg = c->fg; last_bg = c->bg; color_set = true;
            }

            if (c->cp > 0) {
                char utf8[5];
                int  l = utf8_encode(c->cp, utf8);
                buf_write(utf8, l);
            } else {
                buf_write(" ", 1);
            }

            cur_x++;
            c->dirty = false;
        }
    }
    if (buf_pos > 0) write(STDOUT_FILENO, buffer, buf_pos);
}

/* =========================================================
 * 16. Message Log
 * ========================================================= */

static void push_msg(RGB color, const char *fmt, ...)
{
    char buf[80];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    Message *m = &G.messages[G.msg_head % MSG_COUNT];
    strncpy(m->text, buf, sizeof(m->text)-1);
    m->color = color;
    G.msg_head++;
}

/* =========================================================
 * 17. Map Generation (BSP-ish rooms + corridors)
 * ========================================================= */

static int rng(int lo, int hi) { return lo + rand() % (hi - lo + 1); }

static void map_clear(void)
{
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            G.map[y][x].type     = TILE_WALL;
            G.map[y][x].visible  = false;
            G.map[y][x].explored = false;
        }
}

static void carve_room(Room r)
{
    for (int y = r.y; y < r.y + r.h; y++)
        for (int x = r.x; x < r.x + r.w; x++)
            G.map[y][x].type = TILE_FLOOR;
}

static void carve_h(int x1, int x2, int y)
{
    if (x1 > x2) { int t=x1; x1=x2; x2=t; }
    for (int x = x1; x <= x2; x++) G.map[y][x].type = TILE_FLOOR;
}
static void carve_v(int y1, int y2, int x)
{
    if (y1 > y2) { int t=y1; y1=y2; y2=t; }
    for (int y = y1; y <= y2; y++) G.map[y][x].type = TILE_FLOOR;
}

static void connect_rooms(Room a, Room b)
{
    int ax = a.x + a.w/2, ay = a.y + a.h/2;
    int bx = b.x + b.w/2, by = b.y + b.h/2;
    if (rng(0,1)) { carve_h(ax, bx, ay); carve_v(ay, by, bx); }
    else           { carve_v(ay, by, ax); carve_h(ax, bx, by); }
}

static bool rooms_overlap(Room a, Room b)
{
    return !(a.x + a.w + 1 < b.x || b.x + b.w + 1 < a.x ||
             a.y + a.h + 1 < b.y || b.y + b.h + 1 < a.y);
}

static void spawn_enemies_in_room(Room r)
{
    int count = rng(1, 3);
    for (int i = 0; i < count && G.enemy_count < MAX_ENEMIES; i++) {
        int ex = rng(r.x+1, r.x+r.w-2);
        int ey = rng(r.y+1, r.y+r.h-2);

        Enemy *e = &G.enemies[G.enemy_count++];
        e->x     = ex; e->y = ey; e->alive = true;

        int depth_bonus = G.player.depth;
        int roll = rng(0, 99);
        if (roll < 55) {
            e->type    = MOB_GOBLIN;
            e->hp = e->max_hp = 8  + depth_bonus*2;
            e->attack  = 2  + depth_bonus;
            e->defense = 0;
            e->xp_value = 10 + depth_bonus*5;
        } else if (roll < 85) {
            e->type    = MOB_ORC;
            e->hp = e->max_hp = 16 + depth_bonus*3;
            e->attack  = 4  + depth_bonus;
            e->defense = 1  + depth_bonus/2;
            e->xp_value = 25 + depth_bonus*10;
        } else {
            e->type    = MOB_TROLL;
            e->hp = e->max_hp = 30 + depth_bonus*5;
            e->attack  = 7  + depth_bonus*2;
            e->defense = 2  + depth_bonus;
            e->xp_value = 60 + depth_bonus*15;
        }
    }
}

static void spawn_items_in_room(Room r)
{
    if (rng(0,2) == 0 && G.item_count < MAX_ITEMS) {
        Item *it = &G.items[G.item_count++];
        it->x = rng(r.x+1, r.x+r.w-2);
        it->y = rng(r.y+1, r.y+r.h-2);
        it->picked = false;

        int roll = rng(0, 99);
        if      (roll < 35) { it->type = ITEM_HEALTH_POTION;   it->value = 15 + G.player.depth*3; }
        else if (roll < 55) { it->type = ITEM_GOLD;            it->value = rng(5, 30) * (G.player.depth+1); }
        else if (roll < 70) { it->type = ITEM_STRENGTH_POTION; it->value = 3; }
        else if (roll < 82) { it->type = ITEM_SWORD;           it->value = 2 + G.player.depth; }
        else if (roll < 92) { it->type = ITEM_SHIELD;          it->value = 1 + G.player.depth/2; }
        else                { it->type = ITEM_ARMOR;           it->value = 2 + G.player.depth; }
    }
}

static void generate_map(void)
{
    map_clear();
    G.room_count  = 0;
    G.enemy_count = 0;
    G.item_count  = 0;
    memset(G.enemies, 0, sizeof(G.enemies));
    memset(G.items,   0, sizeof(G.items));

    for (int attempt = 0; attempt < 200 && G.room_count < MAX_ROOMS; attempt++) {
        Room r;
        r.w = rng(MIN_ROOM, MAX_ROOM);
        r.h = rng(MIN_ROOM, MAX_ROOM);
        r.x = rng(1, MAP_W - r.w - 2);
        r.y = rng(1, MAP_H - r.h - 2);

        bool ok = true;
        for (int i = 0; i < G.room_count; i++)
            if (rooms_overlap(r, G.rooms[i])) { ok = false; break; }
        if (!ok) continue;

        carve_room(r);
        if (G.room_count > 0) connect_rooms(G.rooms[G.room_count-1], r);

        // Don't spawn in first room (player start)
        if (G.room_count > 0) {
            spawn_enemies_in_room(r);
            spawn_items_in_room(r);
        }

        G.rooms[G.room_count++] = r;
    }

    // Place stairs in last room
    if (G.room_count > 0) {
        Room *last = &G.rooms[G.room_count-1];
        int sx = last->x + last->w/2;
        int sy = last->y + last->h/2;
        if (G.player.depth < MAX_DEPTH)
            G.map[sy][sx].type = TILE_STAIRS_DOWN;
        // On last floor, no stairs (find the artifact!)
    }
}

/* =========================================================
 * 18. FOV (simple raycasting shadow)
 * ========================================================= */

static void fov_clear(void)
{
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            G.map[y][x].visible = false;
}

static bool blocks_sight(int x, int y)
{
    if (x<0||x>=MAP_W||y<0||y>=MAP_H) return true;
    return G.map[y][x].type == TILE_WALL;
}

static void cast_ray(int ox, int oy, float dx, float dy, int range)
{
    float cx = ox + 0.5f, cy = oy + 0.5f;
    for (int i = 0; i < range; i++) {
        cx += dx; cy += dy;
        int ix = (int)cx, iy = (int)cy;
        if (ix<0||ix>=MAP_W||iy<0||iy>=MAP_H) break;
        G.map[iy][ix].visible  = true;
        G.map[iy][ix].explored = true;
        if (blocks_sight(ix, iy)) break;
    }
}

static void compute_fov(int px, int py, int range)
{
    fov_clear();
    G.map[py][px].visible  = true;
    G.map[py][px].explored = true;

    // Cast 360 rays
    for (int i = 0; i < 360; i++) {
        float angle = i * 3.14159265f / 180.0f;
        cast_ray(px, py, cosf(angle), sinf(angle), range);
    }
}

/* =========================================================
 * 19. Player Init & Level Up
 * ========================================================= */

static void player_level_up(void)
{
    G.player.level++;
    G.player.xp_next  = G.player.level * 50;
    G.player.max_hp  += 8;
    G.player.hp       = G.player.max_hp;
    G.player.attack  += 2;
    G.player.defense += 1;
    push_msg(C_UI_TITLE, "*** Level Up! You are now level %d! ***", G.player.level);
}

static void player_gain_xp(int xp)
{
    G.player.xp += xp;
    while (G.player.xp >= G.player.xp_next)
        player_level_up();
}

static int player_init(void)
{
    G.player.x        = G.rooms[0].x + G.rooms[0].w/2;
    G.player.y        = G.rooms[0].y + G.rooms[0].h/2;
    G.player.hp       = 30;
    G.player.max_hp   = 30;
    G.player.attack   = 5;
    G.player.defense  = 1;
    G.player.level    = 1;
    G.player.xp       = 0;
    G.player.xp_next  = 50;
    G.player.gold     = 0;
    G.player.inv_count= 0;
    G.player.depth    = 0;
    memset(G.player.inventory, 0, sizeof(G.player.inventory));
    push_msg(C_MSG_GOOD, "You descend into the dungeon. Good luck, adventurer!");
    push_msg(C_MSG_INFO, "WASD/hjkl:move  p:pickup  i:inventory  q:quit");
    return 0;
}

/* =========================================================
 * 20. Game Init
 * ========================================================= */

static int game_init(void)
{
    srand((unsigned)time(NULL));
    memset(&G.player,   0, sizeof(G.player));
    memset(G.messages, 0, sizeof(G.messages));
    G.msg_head = 0;
    G.ui_mode  = UI_GAME;
    generate_map();
    player_init();
    compute_fov(G.player.x, G.player.y, 10);
    return 0;
}
APP_INIT(game_init);

/* =========================================================
 * 21. Combat
 * ========================================================= */

static int calc_damage(int attack, int defense)
{
    int base = attack - defense;
    if (base < 1) base = 1;
    return base + rng(-1, 2);
}

static void player_attack(Enemy *e)
{
    int dmg = calc_damage(G.player.attack, e->defense);
    e->hp -= dmg;

    const char *names[] = {"", "Goblin", "Orc", "Troll"};
    if (e->hp <= 0) {
        e->alive = false;
        push_msg(C_MSG_GOOD, "You slay the %s! (+%d XP)", names[e->type], e->xp_value);
        player_gain_xp(e->xp_value);
        // Chance to drop gold
        if (rng(0,2)==0 && G.item_count < MAX_ITEMS) {
            Item *it = &G.items[G.item_count++];
            it->x = e->x; it->y = e->y;
            it->type = ITEM_GOLD;
            it->value = rng(3,15);
            it->picked = false;
        }
    } else {
        push_msg(C_MSG_INFO, "You hit the %s for %d damage.", names[e->type], dmg);
    }
}

static void enemy_attack(Enemy *e)
{
    int dmg = calc_damage(e->attack, G.player.defense);
    G.player.hp -= dmg;

    const char *names[] = {"", "Goblin", "Orc", "Troll"};
    push_msg(C_MSG_BAD, "The %s hits you for %d damage!", names[e->type], dmg);

    if (G.player.hp <= 0) {
        G.player.hp = 0;
        G.ui_mode   = UI_GAME_OVER;
    }
}

/* =========================================================
 * 22. AI (simple chase + attack)
 * ========================================================= */

static int sign(int x) { return (x>0)-(x<0); }

static bool enemy_at(int x, int y, int *idx)
{
    for (int i = 0; i < G.enemy_count; i++) {
        if (G.enemies[i].alive && G.enemies[i].x==x && G.enemies[i].y==y) {
            if (idx) *idx = i;
            return true;
        }
    }
    return false;
}

static void enemies_act(void)
{
    for (int i = 0; i < G.enemy_count; i++) {
        Enemy *e = &G.enemies[i];
        if (!e->alive) continue;
        if (!G.map[e->y][e->x].visible) continue; // only act when player can see

        int dx = sign(G.player.x - e->x);
        int dy = sign(G.player.y - e->y);

        int nx = e->x + dx;
        int ny = e->y + dy;

        if (nx == G.player.x && ny == G.player.y) {
            enemy_attack(e);
        } else if (G.map[ny][nx].type == TILE_FLOOR && !enemy_at(nx, ny, NULL)) {
            e->x = nx; e->y = ny;
        }
    }
}

/* =========================================================
 * 23. Item Use
 * ========================================================= */

static const char *item_name(ItemType t)
{
    switch(t) {
        case ITEM_HEALTH_POTION:   return "Health Potion";
        case ITEM_STRENGTH_POTION: return "Strength Potion";
        case ITEM_SWORD:           return "Magic Sword";
        case ITEM_SHIELD:          return "Magic Shield";
        case ITEM_ARMOR:           return "Magic Armor";
        case ITEM_GOLD:            return "Gold Coins";
        default: return "Unknown";
    }
}

static uint32_t item_glyph(ItemType t)
{
    switch(t) {
        case ITEM_HEALTH_POTION:   return '!';
        case ITEM_STRENGTH_POTION: return '!';
        case ITEM_SWORD:           return '/';
        case ITEM_SHIELD:          return ')';
        case ITEM_ARMOR:           return '[';
        case ITEM_GOLD:            return '*';
        default: return '?';
    }
}

static RGB item_color(ItemType t)
{
    switch(t) {
        case ITEM_HEALTH_POTION:   return C_ITEM_POTION;
        case ITEM_STRENGTH_POTION: return C_ITEM_POTION;
        case ITEM_SWORD:           return C_ITEM_SWORD;
        case ITEM_SHIELD:          return C_ITEM_ARMOR;
        case ITEM_ARMOR:           return C_ITEM_ARMOR;
        case ITEM_GOLD:            return C_ITEM_GOLD;
        default: return C_UI_TEXT;
    }
}

static void use_item(int inv_idx)
{
    if (inv_idx < 0 || inv_idx >= G.player.inv_count) return;
    ItemType t = G.player.inventory[inv_idx];

    switch (t) {
        case ITEM_HEALTH_POTION: {
            int heal = 15 + G.player.level*3;
            G.player.hp += heal;
            if (G.player.hp > G.player.max_hp) G.player.hp = G.player.max_hp;
            push_msg(C_MSG_GOOD, "You drink the potion. +%d HP.", heal);
            break;
        }
        case ITEM_STRENGTH_POTION:
            G.player.attack += 3;
            push_msg(C_MSG_GOOD, "Power surges through you! +3 ATK.");
            break;
        case ITEM_SWORD:
            G.player.attack += 2 + G.player.depth;
            push_msg(C_MSG_GOOD, "You equip the sword. +%d ATK.", 2+G.player.depth);
            break;
        case ITEM_SHIELD:
            G.player.defense += 1 + G.player.depth/2;
            push_msg(C_MSG_GOOD, "You equip the shield. +%d DEF.", 1+G.player.depth/2);
            break;
        case ITEM_ARMOR:
            G.player.defense += 2 + G.player.depth;
            push_msg(C_MSG_GOOD, "You don the armor. +%d DEF.", 2+G.player.depth);
            break;
        default:
            push_msg(C_MSG_INFO, "You can't use that here.");
            return;
    }

    // Remove from inventory
    for (int i = inv_idx; i < G.player.inv_count-1; i++)
        G.player.inventory[i] = G.player.inventory[i+1];
    G.player.inv_count--;
    G.ui_mode = UI_GAME;
}

/* =========================================================
 * 24. Player Movement & Actions
 * ========================================================= */

static void try_descend(void)
{
    if (G.map[G.player.y][G.player.x].type != TILE_STAIRS_DOWN) {
        push_msg(C_MSG_INFO, "There are no stairs here.");
        return;
    }
    G.player.depth++;
    if (G.player.depth >= MAX_DEPTH) {
        G.ui_mode = UI_VICTORY;
        return;
    }
    generate_map();
    G.player.x = G.rooms[0].x + G.rooms[0].w/2;
    G.player.y = G.rooms[0].y + G.rooms[0].h/2;
    compute_fov(G.player.x, G.player.y, 10);
    push_msg(C_MSG_INFO, "You descend deeper... Floor %d.", G.player.depth+1);
}

static void try_pickup(void)
{
    for (int i = 0; i < G.item_count; i++) {
        Item *it = &G.items[i];
        if (it->picked || it->x != G.player.x || it->y != G.player.y) continue;

        if (it->type == ITEM_GOLD) {
            G.player.gold += it->value;
            push_msg(C_MSG_GOLD, "You pick up %d gold coins.", it->value);
            it->picked = true;
            return;
        }

        if (G.player.inv_count >= INV_SIZE) {
            push_msg(C_MSG_BAD, "Your inventory is full!");
            return;
        }

        G.player.inventory[G.player.inv_count++] = it->type;
        push_msg(C_MSG_GOOD, "You pick up: %s.", item_name(it->type));
        it->picked = true;
        return;
    }
    push_msg(C_MSG_INFO, "Nothing here to pick up.");
}

static void player_move(int dx, int dy)
{
    if (G.ui_mode != UI_GAME) return;

    int nx = G.player.x + dx;
    int ny = G.player.y + dy;

    if (nx < 0 || nx >= MAP_W || ny < 0 || ny >= MAP_H) return;
    if (G.map[ny][nx].type == TILE_WALL) return;

    // Check enemy
    int eidx;
    if (enemy_at(nx, ny, &eidx)) {
        player_attack(&G.enemies[eidx]);
        enemies_act();
        compute_fov(G.player.x, G.player.y, 10);
        return;
    }

    G.player.x = nx;
    G.player.y = ny;

    enemies_act();
    compute_fov(G.player.x, G.player.y, 10);
}

/* =========================================================
 * 25. Drawing Helpers
 * ========================================================= */

// Draw a string at (x,y) with fg/bg
static void draw_str(int x, int y, const char *s, RGB fg, RGB bg)
{
    for (int i = 0; s[i]; i++)
        set_cell(x+i, y, (uint32_t)(unsigned char)s[i], fg, bg);
}

// Fill a rectangle
static void draw_rect(int x, int y, int w, int h, uint32_t cp, RGB fg, RGB bg)
{
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            set_cell(x+dx, y+dy, cp, fg, bg);
}

// Box-drawing characters
#define BD_H   0x2500
#define BD_V   0x2502
#define BD_TL  0x250C
#define BD_TR  0x2510
#define BD_BL  0x2514
#define BD_BR  0x2518
#define BD_TM  0x252C
#define BD_BM  0x2534
#define BD_LM  0x251C
#define BD_RM  0x2524
#define BD_C   0x253C

static void draw_box(int x, int y, int w, int h, RGB fg, RGB bg)
{
    set_cell(x,     y,     BD_TL, fg, bg);
    set_cell(x+w-1, y,     BD_TR, fg, bg);
    set_cell(x,     y+h-1, BD_BL, fg, bg);
    set_cell(x+w-1, y+h-1, BD_BR, fg, bg);
    for (int i = 1; i < w-1; i++) { set_cell(x+i, y,     BD_H, fg, bg); set_cell(x+i, y+h-1, BD_H, fg, bg); }
    for (int j = 1; j < h-1; j++) { set_cell(x,   y+j,   BD_V, fg, bg); set_cell(x+w-1, y+j, BD_V, fg, bg); }
}

/* =========================================================
 * 26. Map Viewport
 *   The map (MAP_W x MAP_H) is rendered into a viewport.
 *   Layout: left panel=map, right panel=stats+log
 * ========================================================= */

static void draw_map(int vx, int vy, int vw, int vh)
{
    // Center camera on player
    int cam_x = G.player.x - vw/2;
    int cam_y = G.player.y - vh/2;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x + vw > MAP_W) cam_x = MAP_W - vw;
    if (cam_y + vh > MAP_H) cam_y = MAP_H - vh;

    for (int sy = 0; sy < vh; sy++) {
        for (int sx = 0; sx < vw; sx++) {
            int mx = cam_x + sx;
            int my = cam_y + sy;
            int tx = vx + sx;
            int ty = vy + sy;

            if (mx < 0||mx>=MAP_W||my<0||my>=MAP_H) {
                set_cell(tx, ty, ' ', C_BLACK, C_BLACK);
                continue;
            }

            Tile *t = &G.map[my][mx];

            if (!t->explored) {
                set_cell(tx, ty, ' ', C_BLACK, C_BLACK);
                continue;
            }

            RGB dim_factor = t->visible ? (RGB){0,0,0} : (RGB){0,0,0};
            (void)dim_factor;

            // Player
            if (mx == G.player.x && my == G.player.y) {
                set_cell(tx, ty, '@', C_PLAYER, C_FLOOR);
                continue;
            }

            // Enemies (only if visible)
            bool drew_entity = false;
            if (t->visible) {
                for (int i = 0; i < G.enemy_count && !drew_entity; i++) {
                    Enemy *e = &G.enemies[i];
                    if (!e->alive || e->x!=mx || e->y!=my) continue;
                    uint32_t glyph;
                    RGB      ecol;
                    switch(e->type) {
                        case MOB_GOBLIN: glyph='g'; ecol=C_GOBLIN; break;
                        case MOB_ORC:    glyph='o'; ecol=C_ORC;    break;
                        case MOB_TROLL:  glyph='T'; ecol=C_TROLL;  break;
                        default: glyph='?'; ecol=C_UI_TEXT; break;
                    }
                    set_cell(tx, ty, glyph, ecol, C_FLOOR);
                    drew_entity = true;
                }

                // Items (only if visible)
                for (int i = 0; i < G.item_count && !drew_entity; i++) {
                    Item *it = &G.items[i];
                    if (it->picked || it->x!=mx || it->y!=my) continue;
                    set_cell(tx, ty, item_glyph(it->type), item_color(it->type), C_FLOOR);
                    drew_entity = true;
                }
            }
            if (drew_entity) continue;

            // Tiles
            switch (t->type) {
                case TILE_FLOOR: {
                    RGB fg = t->visible ? (RGB){50,50,70} : (RGB){30,30,45};
                    RGB bg = t->visible ? C_FLOOR         : (RGB){20,20,30};
                    set_cell(tx, ty, '.', fg, bg);
                    break;
                }
                case TILE_WALL: {
                    RGB fg = t->visible ? C_WALL_FG       : (RGB){45,40,60};
                    RGB bg = t->visible ? C_WALL           : (RGB){30,25,40};
                    set_cell(tx, ty, '#', fg, bg);
                    break;
                }
                case TILE_STAIRS_DOWN: {
                    RGB fg = t->visible ? C_STAIRS : (RGB){50,100,120};
                    RGB bg = t->visible ? C_FLOOR  : (RGB){20,20,30};
                    set_cell(tx, ty, '>', fg, bg);
                    break;
                }
                default:
                    set_cell(tx, ty, ' ', C_BLACK, C_BLACK);
            }
        }
    }
}

/* =========================================================
 * 27. Stats Panel
 * ========================================================= */

static void draw_stats_panel(int px, int py, int pw, int ph)
{
    // Background
    draw_rect(px, py, pw, ph, ' ', C_UI_TEXT, C_UI_BG);
    draw_box(px, py, pw, ph, C_UI_BORDER, C_UI_BG);

    int row = py + 1;
    int cx  = px + 2;

    // Title
    draw_str(cx, row++, "  DUNGEON CRAWLER  ", C_UI_TITLE, C_UI_BG);
    row++;

    // Floor
    char buf[64];
    snprintf(buf, sizeof(buf), "Floor: %d / %d", G.player.depth+1, MAX_DEPTH);
    draw_str(cx, row++, buf, C_UI_TEXT, C_UI_BG);
    row++;

    // HP bar
    RGB hp_col = G.player.hp > G.player.max_hp*2/3 ? C_HP_HIGH :
                 G.player.hp > G.player.max_hp/3    ? C_HP_MED  : C_HP_LOW;

    snprintf(buf, sizeof(buf), "HP: %3d / %3d", G.player.hp, G.player.max_hp);
    draw_str(cx, row++, buf, hp_col, C_UI_BG);

    // HP bar graphic
    int bar_w = pw - 4;
    int filled = bar_w * G.player.hp / G.player.max_hp;
    for (int i = 0; i < bar_w; i++) {
        uint32_t ch = 0x2588; // full block
        RGB      fc = i < filled ? hp_col : (RGB){40,35,55};
        set_cell(cx+i, row, ch, fc, C_UI_BG);
    }
    row += 2;

    // XP
    snprintf(buf, sizeof(buf), "XP: %d / %d", G.player.xp, G.player.xp_next);
    draw_str(cx, row++, buf, (RGB){150,220,255}, C_UI_BG);

    // XP bar
    int xpf = bar_w * G.player.xp / G.player.xp_next;
    for (int i = 0; i < bar_w; i++) {
        uint32_t ch = 0x2588;
        RGB      fc = i < xpf ? (RGB){100,180,255} : (RGB){30,40,55};
        set_cell(cx+i, row, ch, fc, C_UI_BG);
    }
    row += 2;

    // Stats
    draw_str(cx, row++, "--- STATS ---", C_UI_BORDER, C_UI_BG);
    snprintf(buf, sizeof(buf), "Level:  %d", G.player.level);
    draw_str(cx, row++, buf, C_UI_TEXT, C_UI_BG);
    snprintf(buf, sizeof(buf), "ATK:    %d", G.player.attack);
    draw_str(cx, row++, buf, C_ITEM_SWORD, C_UI_BG);
    snprintf(buf, sizeof(buf), "DEF:    %d", G.player.defense);
    draw_str(cx, row++, buf, C_ITEM_ARMOR, C_UI_BG);
    snprintf(buf, sizeof(buf), "Gold:   %d", G.player.gold);
    draw_str(cx, row++, buf, C_ITEM_GOLD, C_UI_BG);
    row++;

    // Inventory summary
    draw_str(cx, row++, "--- ITEMS ---", C_UI_BORDER, C_UI_BG);
    snprintf(buf, sizeof(buf), "Slots: %d/%d  (i)", G.player.inv_count, INV_SIZE);
    draw_str(cx, row++, buf, C_UI_TEXT, C_UI_BG);
    row++;

    // Controls
    draw_str(cx, row++, "--- KEYS ---",  C_UI_BORDER, C_UI_BG);
    draw_str(cx, row++, "Move: WASD/hjkl", C_UI_TEXT, C_UI_BG);
    draw_str(cx, row++, "p: pick up",     C_UI_TEXT, C_UI_BG);
    draw_str(cx, row++, "i: inventory",   C_UI_TEXT, C_UI_BG);
    draw_str(cx, row++, ">: descend",     C_UI_TEXT, C_UI_BG);
    draw_str(cx, row++, ".  : wait",      C_UI_TEXT, C_UI_BG);
    draw_str(cx, row++, "q: quit",        C_UI_TEXT, C_UI_BG);
}

/* =========================================================
 * 28. Message Log
 * ========================================================= */

static void draw_msg_log(int px, int py, int pw, int ph)
{
    draw_rect(px, py, pw, ph, ' ', C_UI_TEXT, C_UI_BG);
    draw_box(px, py, pw, ph, C_UI_BORDER, C_UI_BG);
    draw_str(px+2, py, " LOG ", C_UI_TITLE, C_UI_BG);

    int count = MSG_COUNT < ph-2 ? MSG_COUNT : ph-2;
    int start = G.msg_head - count;
    if (start < 0) start = 0;
    int shown = G.msg_head - start;

    for (int i = 0; i < shown; i++) {
        Message *m = &G.messages[(start+i) % MSG_COUNT];
        if (m->text[0] == 0) continue;
        // Clip to panel width
        char clipped[80];
        int  maxw = pw - 3;
        strncpy(clipped, m->text, maxw);
        clipped[maxw] = 0;
        draw_str(px+2, py+1+i, clipped, m->color, C_UI_BG);
    }
}

/* =========================================================
 * 29. Inventory Screen
 * ========================================================= */

static void draw_inventory(void)
{
    if (!G.screen) return;
    int sw = G.screen->width, sh = G.screen->height;

    // Dim background
    for (int y=0;y<sh;y++)
        for (int x=0;x<sw;x++)
            set_cell(x,y,' ',C_UI_BG,C_UI_BG);

    int bw = 40, bh = INV_SIZE + 6;
    int bx = (sw-bw)/2, by = (sh-bh)/2;
    draw_rect(bx,by,bw,bh,' ',C_UI_TEXT,C_UI_BG);
    draw_box(bx,by,bw,bh,C_UI_BORDER,C_UI_BG);
    draw_str(bx+2, by, " INVENTORY ", C_UI_TITLE, C_UI_BG);

    char buf[64];
    snprintf(buf, sizeof(buf), "Gold: %d", G.player.gold);
    draw_str(bx+2, by+1, buf, C_ITEM_GOLD, C_UI_BG);

    for (int i = 0; i < INV_SIZE; i++) {
        int row = by + 3 + i;
        if (row >= by+bh-1) break;

        if (i < G.player.inv_count) {
            ItemType t = G.player.inventory[i];
            snprintf(buf, sizeof(buf), " %c) %s", 'a'+i, item_name(t));
            draw_str(bx+2, row, buf, item_color(t), C_UI_BG);
        } else {
            draw_str(bx+2, row, " -- empty --", (RGB){60,55,80}, C_UI_BG);
        }
    }
    draw_str(bx+2, by+bh-2, "a-z: use item   ESC: close", C_UI_TEXT, C_UI_BG);
}

/* =========================================================
 * 30. Game Over / Victory Screens
 * ========================================================= */

static void draw_game_over(void)
{
    if (!G.screen) return;
    int sw = G.screen->width, sh = G.screen->height;
    for (int y=0;y<sh;y++)
        for (int x=0;x<sw;x++)
            set_cell(x,y,' ',(RGB){40,10,10},(RGB){10,5,5});

    int cx = sw/2, cy = sh/2;
    draw_str(cx-10, cy-3, "  *** GAME OVER ***  ", (RGB){220,60,60}, (RGB){10,5,5});
    char buf[64];
    snprintf(buf, sizeof(buf), "You died on floor %d.", G.player.depth+1);
    draw_str(cx-14, cy-1, buf, C_UI_TEXT, (RGB){10,5,5});
    snprintf(buf, sizeof(buf), "Level %d  |  Gold: %d  |  XP: %d", G.player.level, G.player.gold, G.player.xp);
    draw_str(cx-18, cy+1, buf, C_UI_TEXT, (RGB){10,5,5});
    draw_str(cx-10, cy+3, "Press 'q' to quit.", (RGB){180,180,180}, (RGB){10,5,5});
}

static void draw_victory(void)
{
    if (!G.screen) return;
    int sw = G.screen->width, sh = G.screen->height;
    for (int y=0;y<sh;y++)
        for (int x=0;x<sw;x++)
            set_cell(x,y,' ',(RGB){10,30,10},(RGB){5,10,5});

    int cx = sw/2, cy = sh/2;
    draw_str(cx-12, cy-4, " *** VICTORY! *** ", C_UI_TITLE, (RGB){5,10,5});
    draw_str(cx-18, cy-2, "You have conquered the dungeon and found the Artifact!", C_MSG_GOOD, (RGB){5,10,5});
    char buf[64];
    snprintf(buf, sizeof(buf), "Level %d  |  Gold: %d  |  XP: %d", G.player.level, G.player.gold, G.player.xp);
    draw_str(cx-18, cy, buf, C_UI_TEXT, (RGB){5,10,5});
    draw_str(cx-10, cy+2, "Press 'q' to quit.", (RGB){180,180,180}, (RGB){5,10,5});
}

/* =========================================================
 * 31. Main Draw
 * ========================================================= */

static void draw_all(void)
{
    if (!G.screen) return;
    int sw = G.screen->width, sh = G.screen->height;

    if (G.ui_mode == UI_GAME_OVER)  { draw_game_over(); return; }
    if (G.ui_mode == UI_VICTORY)    { draw_victory();   return; }
    if (G.ui_mode == UI_INVENTORY)  { draw_inventory(); return; }

    // Layout:
    //   [  MAP VIEWPORT  ] [ STATS ]
    //   [   MSG LOG      ] [ STATS ]

    int stats_w = 24;
    int log_h   = MSG_COUNT + 2;
    int map_w   = sw - stats_w;
    int map_h   = sh - log_h;

    if (map_w < 20 || map_h < 10) {
        // Terminal too small
        draw_str(0, 0, "Terminal too small! Please resize.", C_MSG_BAD, C_BLACK);
        return;
    }

    draw_map(0, 0, map_w, map_h);
    draw_stats_panel(map_w, 0, stats_w, sh);
    draw_msg_log(0, map_h, map_w, log_h);
}

/* =========================================================
 * 32. Input Handling
 * ========================================================= */

static void handle_input(char ch)
{
    if (G.ui_mode == UI_INVENTORY) {
        if (ch == 27 || ch == 'i') { G.ui_mode = UI_GAME; return; } // ESC or i
        if (ch >= 'a' && ch < 'a' + G.player.inv_count)
            use_item(ch - 'a');
        return;
    }

    if (G.ui_mode == UI_GAME_OVER || G.ui_mode == UI_VICTORY) {
        if (ch == 'q') G.is_running = false;
        return;
    }

    // Escape sequences (arrow keys)
    // We read them in a multi-byte sequence below in main
    switch (ch) {
        case 'w': case 'k': player_move( 0,-1); break;
        case 's': case 'j': player_move( 0, 1); break;
        case 'a': case 'h': player_move(-1, 0); break;
        case 'd': case 'l': player_move( 1, 0); break;
        case 'y': player_move(-1,-1); break;
        case 'u': player_move( 1,-1); break;
        case 'b': player_move(-1, 1); break;
        case 'n': player_move( 1, 1); break;
        case '.': enemies_act(); compute_fov(G.player.x, G.player.y, 10); break;
        case 'p': try_pickup(); break;
        case '>': try_descend(); break;
        case 'i': G.ui_mode = UI_INVENTORY; break;
        case 'q': G.is_running = false; break;
    }
}

/* =========================================================
 * 33. Main
 * ========================================================= */

int main(void)
{
    G.is_running = true;

    for (initcall_t *p = __start_initcalls; p < __stop_initcalls; ++p) {
        if ((*p)() != 0) {
            fprintf(stderr, "Init failed!\n");
            return 1;
        }
    }

    char seq[4];

    while (G.is_running) {
        if (G.win_resized) {
            struct winsize ws;
            ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
            screen_resize(ws.ws_col, ws.ws_row);
            // Mark full redraw
            for (int i = 0; i < G.screen->width * G.screen->height; i++)
                G.screen->cells[i].dirty = true;
            G.win_resized = 0;
        }

        fd_set fds;
        struct timeval tv = {0, 50000}; // 50ms = 20 FPS
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        if (select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0) {
            int n = read(STDIN_FILENO, seq, 1);
            if (n > 0) {
                if (seq[0] == 27) {
                    // Possible escape sequence
                    tv = (struct timeval){0, 5000};
                    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
                    if (select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0) {
                        read(STDIN_FILENO, seq+1, 1);
                        if (seq[1] == '[') {
                            FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
                            if (select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) > 0) {
                                read(STDIN_FILENO, seq+2, 1);
                                switch(seq[2]) {
                                    case 'A': player_move(0,-1); break; // up
                                    case 'B': player_move(0, 1); break; // down
                                    case 'C': player_move(1, 0); break; // right
                                    case 'D': player_move(-1,0); break; // left
                                }
                            }
                        }
                    } else {
                        // Lone ESC
                        handle_input(27);
                    }
                } else {
                    handle_input(seq[0]);
                }
            }
        }

        draw_all();
        screen_flush();
    }

    terminal_restore();
    printf("\nThanks for playing! Your stats:\n");
    printf("  Level:  %d\n",   G.player.level);
    printf("  Gold:   %d\n",   G.player.gold);
    printf("  XP:     %d\n",   G.player.xp);
    printf("  Floor:  %d/%d\n", G.player.depth+1, MAX_DEPTH);
    return 0;
}
