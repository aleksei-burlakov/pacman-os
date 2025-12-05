#include "engine.h"
#include <arch/i686/isr.h>
#include <debug.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_ROWS                29
#define NUM_COLS                28
// Стандартная 16-цветная палитра VGA (подойдёт и для 12h)
enum VGA_COLOR {
    VGA_COL_BLACK        = 0x0,
    VGA_COL_BLUE         = 0x1,
    VGA_COL_GREEN        = 0x2,
    VGA_COL_CYAN         = 0x3,
    VGA_COL_RED          = 0x4,
    VGA_COL_MAGENTA      = 0x5,
    VGA_COL_BROWN        = 0x6,
    VGA_COL_LIGHT_GRAY   = 0x7,
    VGA_COL_DARK_GRAY    = 0x8,
    VGA_COL_LIGHT_BLUE   = 0x9,
    VGA_COL_LIGHT_GREEN  = 0xA,
    VGA_COL_LIGHT_CYAN   = 0xB,
    VGA_COL_LIGHT_RED    = 0xC,
    VGA_COL_LIGHT_MAGENTA= 0xD,
    VGA_COL_YELLOW       = 0xE,
    VGA_COL_WHITE        = 0xF,
};

#define TILE_W     (FB_WIDTH  / NUM_COLS)   // 640 / 28
#define TILE_H     (FB_HEIGHT / NUM_ROWS)   // 480 / 29

// BEGIN: copied from i8259.c
#define PIC1_COMMAND_PORT           0x20
#define PIC_CMD_END_OF_INTERRUPT    0x20
// END: copied from i8259.c

#define MODULE  "PACMAN"
#define IRQ0_PERIOD             11  // trigger timer every 15th tick

#define FB_WIDTH   640
#define FB_HEIGHT  480

#define PACMAN_SYMBOL 'C'
#define GHOST_SYMBOL 'G'

static uint8_t g_framebuffer[FB_WIDTH * FB_HEIGHT];

static inline void fb_put_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    g_framebuffer[y * FB_WIDTH + x] = color & 0x0F;  // 4-bit color
}

#define VGA_MEM ((uint8_t*)0xA0000)

static inline void vga_set_map_mask(uint8_t mask)
{
    i686_outb(0x3C4, 0x02);  // Sequencer index 2 = Map Mask
    i686_outb(0x3C5, mask);
}

void vga_blit_framebuffer_12h(void)
{
    // We treat VRAM plane-per-plane, byte-per-byte.
    // Each byte in a plane represents 8 pixels horizontally.
    // 1 byte == 8 pixels. --> we iterate pixelwise.
    for (int y = 0; y < FB_HEIGHT; ++y) {
        int fb_row_off = y * FB_WIDTH; // frame buffer offeset
        int vram_row_off = y * 80;     // 640 / 8 = 80 bytes per scanline

        for (int byte_x = 0; byte_x < 80; ++byte_x) {
            int fb_x = byte_x * 8;     // 8 pixels per byte

            // Build one byte per plane
            uint8_t plane_bytes[4] = {0, 0, 0, 0};

            for (int bit = 0; bit < 8; ++bit) {
                int x = fb_x + bit;
                if (x >= FB_WIDTH) break;

                uint8_t color = g_framebuffer[fb_row_off + x];

                // For each of the 4 planes, set bit if that color bit is 1
                for (int plane = 0; plane < 4; ++plane) {
                    if (color & (1 << plane)) {
                        plane_bytes[plane] |= (uint8_t)(0x80 >> bit);
                    }
                }
            }

            // Write byte for each plane
            int vram_off = vram_row_off + byte_x;
            for (int plane = 0; plane < 4; ++plane) {
                vga_set_map_mask((uint8_t)(1 << plane));
                VGA_MEM[vram_off] = plane_bytes[plane];
            }
        }
    }
}

static void fb_clear(uint8_t color)
{
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            fb_put_pixel(x, y, color);
}

static inline void fb_dot(int cx, int cy, uint8_t color, int radius)
{
    const int r  = radius;          // радиус "жирной" точки
    const int r2 = r * r;      // r^2, чтобы не считать sqrt

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            // принадлежит ли точка (dx,dy) кругу радиуса r?
            if (dx*dx + dy*dy <= r2) {
                fb_put_pixel(cx + dx, cy + dy, color);
            }
        }
    }
}

static void fb_line(int x0, int y0, int x1, int y1, uint8_t color)
{
    int dx = x1 - x0;
    int dy = y1 - y0;

    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;

    dx = (dx >= 0) ? dx : -dx;
    dy = (dy >= 0) ? dy : -dy;

    int err = dx - dy;

    while (1) {
        fb_put_pixel(x0, y0, color);

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void fb_rectfill(int x0, int y0, int x1, int y1, uint8_t color)
{
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (y0 > y1) { int t = y0; y0 = y1; y1 = t; }

    if (x1 < 0 || x0 >= FB_WIDTH || y1 < 0 || y0 >= FB_HEIGHT) return;

    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= FB_WIDTH) x1 = FB_WIDTH - 1;
    if (y1 >= FB_HEIGHT) y1 = FB_HEIGHT - 1;

    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x)
            fb_put_pixel(x, y, color);
}

static void fb_circle(int cx, int cy, int r, uint8_t color)
{
    int x = r;
    int y = 0;
    int err = 1 - r;

    while (x >= y) {
        fb_put_pixel(cx + x, cy + y, color);
        fb_put_pixel(cx + y, cy + x, color);
        fb_put_pixel(cx - y, cy + x, color);
        fb_put_pixel(cx - x, cy + y, color);
        fb_put_pixel(cx - x, cy - y, color);
        fb_put_pixel(cx - y, cy - x, color);
        fb_put_pixel(cx + y, cy - x, color);
        fb_put_pixel(cx + x, cy - y, color);

        y++;
        if (err < 0) {
            err += 2*y + 1;
        } else {
            x--;
            err += 2*(y - x + 1);
        }
    }
}

struct Actor {
    int pos_y;
    int pos_x;

    // We keep the last position to prefer going forward
    // rather that back
    int last_pos_y;
    int last_pos_x;
    uint8_t color;
} ghost5, ghost6, ghost7, ghost8, pacman;

int initial_landscape[NUM_ROWS][NUM_COLS] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 3, 1, 0, 0, 1, 2, 1, 0, 0, 0, 1, 2, 1, 1, 2, 1, 0, 0, 0, 1, 2, 1, 0, 0, 1, 3, 1 },
    { 1, 2, 1, 0, 0, 1, 2, 1, 0, 0, 0, 1, 2, 1, 1, 2, 1, 0, 0, 0, 1, 2, 1, 0, 0, 1, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 4, 4, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 0, 0, 0, 6, 5, 0, 1, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 8, 7, 0, 0, 0, 1, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 3, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 9, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 3, 1 },
    { 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1 },
    { 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};


char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0
};

typedef enum { left=0, right=1, up=2, down=3 } Direction;

int game_window[NUM_ROWS][NUM_COLS];
Direction RandomDirection();

static int support_rdrand = false;

void DrawGhost(struct Actor ghost)
{
    // TODO: check collisions
    //VGA_putchr(ghost.pos_x, ghost.pos_y, ghost.symbol);
    //VGA_putcolor(ghost.pos_x, ghost.pos_y, ghost.color);

    int px0 = ghost.pos_x * TILE_W;
    int py0 = ghost.pos_y * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;

    fb_rectfill(px0, py0, px1, py1, VGA_COL_BLACK);

    int cx = px0 + TILE_W / 2;
    int cy = py0 + TILE_H / 2;
    int r  = (TILE_W < TILE_H ? TILE_W : TILE_H) / 3;
    int r2 = r * r;

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx*dx + dy*dy <= r2) {
                fb_put_pixel(cx + dx, cy + dy, ghost.color);
            }
        }
    }
}

void DrawPacman(struct Actor pacman)
{
    int px0 = pacman.pos_x * TILE_W;
    int py0 = pacman.pos_y * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;

    fb_rectfill(px0, py0, px1, py1, VGA_COL_BLACK);

    int cx = px0 + TILE_W / 2;
    int cy = py0 + TILE_H / 2;
    int r  = (TILE_W < TILE_H ? TILE_W : TILE_H) / 3;  // Пакман покрупнее, чем обычная точка
    int r2 = r * r;

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx*dx + dy*dy <= r2) {
                fb_put_pixel(cx + dx, cy + dy, VGA_COL_YELLOW);
            }
        }
    }
}

static inline clear_cell(int game_x, int game_y)
{
    int px0 = game_x * TILE_W;
    int py0 = game_y * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;
    fb_rectfill(px0, py0, px1, py1, VGA_COL_BLACK);
}

static inline dot_cell(int game_x, int game_y, int raduis)
{
    clear_cell(game_x, game_y);

    int px0 = game_x * TILE_W;
    int py0 = game_y * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;
    fb_dot(px0 + TILE_W/2, py0 + TILE_H/2, VGA_COL_WHITE, raduis);
}

static inline wall_cell(int game_x, int game_y, uint8_t color)
{
    int px0 = game_x * TILE_W;
    int py0 = game_y * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;
    fb_rectfill(px0, py0, px1, py1, color);
}

static void DrawWindow()
{
    //log_vfprintf(VFS_FD_STDOUT, fmt, args);
    for (int y = 0; y < NUM_ROWS; y++) {
        for (int x = 0; x < NUM_COLS; x++) {
            switch (game_window[y][x]){
            case 0: clear_cell(x, y); break; // empty road
            case 1: wall_cell(x, y, VGA_COL_BLUE); break; // wall
            case 2: dot_cell(x, y, 1); break; // small dot
            case 3: dot_cell(x, y, 3); break; // big dot
            case 4: wall_cell(x, y, VGA_COL_GREEN); break; // ghost gate
            default: clear_cell(x, y); break;
            }
        }
    }

    DrawGhost(ghost5);
    //DrawGhost(ghost6);
    //DrawGhost(ghost7);
    //DrawGhost(ghost8);
    DrawPacman(pacman);
    vga_blit_framebuffer_12h();
}


void MovePacman(Direction direction)
{
    switch(direction) {
        case left: // right-->left
            if (game_window[pacman.pos_y][pacman.pos_x-1] != 1) {
                pacman.last_pos_x = pacman.pos_x;
                pacman.pos_x--;
            }
            break;
        case right: // left-->right
            if (game_window[pacman.pos_y][pacman.pos_x+1] != 1) {
                pacman.last_pos_x = pacman.pos_x;
                pacman.pos_x++;
            }
            break;
        case up: // down-->up
            if (game_window[pacman.pos_y-1][pacman.pos_x] != 1) {
                pacman.last_pos_y = pacman.pos_y;
                pacman.pos_y--;
            }
            break;
        case down: // up-->down
            if (game_window[pacman.pos_y+1][pacman.pos_x] != 1) {
                pacman.last_pos_y = pacman.pos_y;
                pacman.pos_y++;
            }
            break;
        default:
            log_err("pacman-kbd", "MOVE PACMAN, DEFAULT direction=%d", direction);
        break;
    }
    DrawWindow();
}


void MoveGhost(struct Actor* ghost)
{
    bool mooved = false;
    if ((game_window[ghost->pos_y+1][ghost->pos_x] == 1)
        && (game_window[ghost->pos_y-1][ghost->pos_x] == 1)
        && (game_window[ghost->pos_y][ghost->pos_x+1] == 1)
        && (game_window[ghost->pos_y][ghost->pos_x-1] == 1)) {
            log_debug("PACMAN", "The ghost is trapped. How is that possible?");
    }
    
    // if we can go straight go straight
    /*if (ghost->last_pos_y != ghost->pos_y) {
        if (ghost->last_pos_y < ghost->pos_y) { // up-->down
            if (game_window[ghost->pos_y+1][ghost->pos_x] != 1) {
                ghost->last_pos_y = ghost->pos_y;
                ghost->pos_y++;
                return;
            }
        } else { // down-->up
            if (game_window[ghost->pos_y-1][ghost->pos_x] != 1) {
                ghost->last_pos_y = ghost->pos_y;
                ghost->pos_y--;
                return;
            }
        }
    } else {
        if (ghost->last_pos_x < ghost->pos_x) { // right-->left
            if (game_window[ghost->pos_y][ghost->pos_x-1] != 1) {
                ghost->last_pos_x = ghost->pos_x;
                ghost->pos_x--;
                return;
            }
        } else { // left-->right
            if (game_window[ghost->pos_y][ghost->pos_x+1] != 1) {
                ghost->last_pos_x = ghost->pos_x;
                ghost->pos_x++;
                return;
            }
        }
    }*/
    while (false == mooved) {
        Direction dir = RandomDirection();
        switch(dir) {
        case left: // move left
            if (game_window[ghost->pos_y][ghost->pos_x-1] != 1) {
                ghost->last_pos_x = ghost->pos_x;
                ghost->pos_x--;
                mooved = true;
            }
            break;
        case right: // move right
            if (game_window[ghost->pos_y][ghost->pos_x+1] != 1) {
                ghost->last_pos_x = ghost->pos_x;
                ghost->pos_x++;
                mooved = true;
            }
            break;
        case up: // move up
            if (game_window[ghost->pos_y-1][ghost->pos_x] != 1) {
                ghost->last_pos_y = ghost->pos_y;
                ghost->pos_y--;
                mooved = true;
            }
            break;
        case down: // move down
            if (game_window[ghost->pos_y+1][ghost->pos_x] != 1) {
                ghost->last_pos_y = ghost->pos_y;
                ghost->pos_y++;
                mooved = true;
            }
            break;
        }
    }
}

bool its_time = false;

void Wait()
{
    while(false == its_time);
    its_time = false;
}

bool has_rdrand() {
    uint32_t eax, ecx;
    __asm__ volatile ("cpuid" : "=a" (eax), "=c" (ecx) : "a" (1));
    return (ecx >> 30) & 1;  // Check RDRAND support (bit 30 in ECX)
}

// rdrand doesn't need a seed
uint32_t rdrand() {
    uint32_t val;
    uint8_t success;
    __asm__ volatile ("rdrand %0; setc %1" : "=r" (val), "=qm" (success));
    return success ? val : 0;  // Return 0 if failed
}

// rdseed is the same as rdrand, but better cryptographic level
uint32_t rdseed() {
    uint32_t val;
    uint8_t success;
    __asm__ volatile ("rdseed %0; setc %1" : "=r" (val), "=qm" (success));
    return success ? val : 0;
}

static uint32_t seed = 0;  // Initialize seed

void seed_rng(void) {
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a"(low), "=d"(high));  // Read TSC
    seed = low ^ high;  // Mix high and low bits
}

uint32_t xorshift(void) {
    seed_rng();
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
}

// Rocket-scince true-random generator between [0 and 3]
Direction RandomDirection()
{
    uint32_t rnd = 0;
    if (support_rdrand) {
        rnd = rdrand();
    }
    else {
        rnd = xorshift();
    }
    return (int)rnd % 4;
}


#define VGA_ADDRESS ((volatile uint8_t*) 0xA0000)
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

void MainLoop()
{
    for (int i = 0; i < 3; i++) {
        log_debug("PACMAN", "Ok, we are in the main loop");
        DrawWindow();
        //MoveGhost(&ghost5);
        Wait();
    }
}

void irq0_handler_timer(Registers* regs)
{
    static int tick = 0;
    tick++;
    if (tick == IRQ0_PERIOD) {
        //log_warn(MODULE, "Unhandled HUI IRQ %d...", 0);
        tick = 0;
        its_time = true;
    }
}

void irq1_handler_keyboard(Registers* regs)
{
    static uint8_t last_code = 0;
    uint8_t scancode = i686_inb(0x60);

    if (scancode == 0xE0) {
        last_code = 0xE0;  // Mark that an extended key is coming
        return;
    }

    if (last_code == 0xE0) {
        // Handle extended keys (e.g., arrow keys)
        switch (scancode) {
            case 0x4B:
                log_debug("pacman-kbd", "Left Arrow Key Pressed");
                MovePacman(left);
                break;
            case 0x4D:
                log_debug("pacman-kbd", "Right Arrow Key Pressed");
                MovePacman(right);
                break;
            case 0x48:
                log_debug("pacman-kbd", "Up Arrow Key Pressed");
                MovePacman(up);
                break;
            case 0x50:
                log_debug("pacman-kbd", "Down Arrow Key Pressed");
                MovePacman(down);
                break;
            // Handle break codes if needed
            case 0xCB: log_debug("pacman-kbd", "Left Arrow Key Released"); break;
            case 0xCD: log_debug("pacman-kbd", "Right Arrow Key Released"); break;
            case 0xC8: log_debug("pacman-kbd", "Up Arrow Key Released"); break;
            case 0xD0: log_debug("pacman-kbd", "Down Arrow Key Released"); break;
            default: log_debug("pacman-kbd", "Unknown Extended Key: 0x%X", scancode); break;
        }
        last_code = 0;  // Reset extended key flag
    } else {
        log_debug("pacman-kbd", "Regular key pressed: Scan Code = 0x%X, as char('%c')"
            , scancode, scancode_to_ascii[scancode]);
    }


    i686_outb(PIC1_COMMAND_PORT, PIC_CMD_END_OF_INTERRUPT);
    i686_iowait(); // optional
}

void Initialize()
{
    // 1. Setup the timer
    i686_IRQ_RegisterHandler(0, irq0_handler_timer);
    i686_IRQ_RegisterHandler(1, irq1_handler_keyboard);

    // 2. Check if the CPU has PRNG
    support_rdrand = has_rdrand();
    if (!support_rdrand) {
        log_err("pacman-rnd", "The CPU doesn't provide rdrand/rdseed");
    }
    
    // 2. Initialize ghosts
    for (int y = 0; y < NUM_ROWS; y++) {
        for (int x = 0; x < NUM_COLS; x++) {
            switch ( initial_landscape[y][x] ) {
            case 5: ghost5.pos_y = y; ghost5.last_pos_y = y; ghost5.pos_x = x; ghost5.last_pos_x = x; ghost5.color = VGA_COL_RED; break;
            case 6: ghost6.pos_y = y; ghost6.last_pos_y = y; ghost6.pos_x = x; ghost6.last_pos_x = x; ghost6.color = VGA_COL_CYAN; break;
            case 7: ghost7.pos_y = y; ghost7.last_pos_y = y; ghost7.pos_x = x; ghost7.last_pos_x = x; ghost7.color = VGA_COL_MAGENTA; break;
            case 8: ghost8.pos_y = y; ghost8.last_pos_y = y; ghost8.pos_x = x; ghost8.last_pos_x = x; ghost8.color = VGA_COL_YELLOW; break;
            case 9: pacman.pos_y = y; pacman.last_pos_y = y; pacman.pos_x = x; pacman.last_pos_x = x; pacman.color = VGA_COL_YELLOW; break;
            default: game_window[y][x] = initial_landscape[y][x];
            }
        }
    }
}

void StartGame()
{
    Initialize();
    MainLoop();
}
