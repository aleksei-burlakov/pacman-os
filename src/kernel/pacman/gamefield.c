#include "gamefield.h"
#include "landscape.h"
#include "utils.h"
#include <stdbool.h>

static inline void clear_cell(int game_x, int game_y)
{
    int px0 = game_x * TILE_W;
    int py0 = (game_y + TOP_OFFSET) * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;
    fb_rectfill(px0, py0, px1, py1, VGA_COL_BLACK);
}

static inline void dot_cell(int game_x, int game_y, int raduis)
{
    clear_cell(game_x, game_y);

    int px0 = game_x * TILE_W;
    int py0 = (game_y + TOP_OFFSET) * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;
    fb_dot(px0 + TILE_W/2, py0 + TILE_H/2, VGA_COL_WHITE, raduis);
}

static inline void wall_cell(int game_x, int game_y, uint8_t color)
{
    int px0 = game_x * TILE_W;
    int py0 = (game_y + TOP_OFFSET) * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;
    fb_rectfill(px0, py0, px1, py1, color);
}

void DrawGhost(struct Actor ghost)
{
    // TODO: check collisions
    //VGA_putchr(ghost.pos_x, ghost.pos_y, ghost.symbol);
    //VGA_putcolor(ghost.pos_x, ghost.pos_y, ghost.color);

    int px0 = ghost.pos_x * TILE_W;
    int py0 = (ghost.pos_y + TOP_OFFSET) * TILE_H;
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
    // 1. Clear the cell with pacman
    clear_cell(pacman.pos_x, pacman.pos_y);

    // 2. Get the vga tile coordinates based on the game position
    int px0 = pacman.pos_x * TILE_W;
    int py0 = (pacman.pos_y + TOP_OFFSET) * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;

    // 3. Get the vga center and radius of pacman
    int cx = px0 + TILE_W / 2;
    int cy = py0 + TILE_H / 2;
    int r  = (TILE_W < TILE_H ? TILE_W : TILE_H) / 3;
    int r2 = r * r;

    // 4. Get the movement direction from last_pos -> pos
    int dx_dir = pacman.pos_x - pacman.last_pos_x;
    int dy_dir = pacman.pos_y - pacman.last_pos_y;

    enum { DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN } dir = DIR_RIGHT;

    if (dx_dir > 0)      dir = DIR_RIGHT;
    else if (dx_dir < 0) dir = DIR_LEFT;
    else if (dy_dir > 0) dir = DIR_DOWN;
    else if (dy_dir < 0) dir = DIR_UP;
    
    // 5. If the pacman is moving at all
    bool moving = (dx_dir != 0 || dy_dir != 0);

    // 5. If the mouth is open (switch between open/close)
    bool mouth_open = moving && ((g_tick & 1) != 0);

    // 6. Draw the circle with/without the mouth
    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx*dx + dy*dy > r2)
                continue;   // вне круга

            if (mouth_open) {
                bool in_mouth = false;

                switch (dir) {
                    case DIR_RIGHT:
                        // вырезаем клин справа
                        if (dx > 0 && my_abs(dy) < dx)
                            in_mouth = true;
                        break;
                    case DIR_LEFT:
                        // клин слева
                        if (dx < 0 && my_abs(dy) < -dx)
                            in_mouth = true;
                        break;
                    case DIR_UP:
                        // клин вверх
                        if (dy < 0 && my_abs(dx) < -dy)
                            in_mouth = true;
                        break;
                    case DIR_DOWN:
                        // клин вниз
                        if (dy > 0 && my_abs(dx) < dy)
                            in_mouth = true;
                        break;
                }

                if (in_mouth)
                    continue;   // не рисуем пиксели в рту
            }

            fb_put_pixel(cx + dx, cy + dy, VGA_COL_YELLOW);
        }
    }
}

void DrawCherry(int game_x, int game_y)
{
    // 1. Clear the tile first
    clear_cell(game_x, game_y);

    // 2. Tile → pixel coordinates
    int px0 = game_x * TILE_W;
    int py0 = (game_y + TOP_OFFSET) * TILE_H;
    int px1 = px0 + TILE_W - 1;
    int py1 = py0 + TILE_H - 1;

    // 3. Basic geometry for the two fruits
    int tile_w = px1 - px0 + 1;
    int tile_h = py1 - py0 + 1;

    int radius = (tile_w < tile_h ? tile_w : tile_h) / 5;   // small-ish circles
    if (radius < 2) radius = 2;

    // Cherry centers (left & right)
    int cx_left  = px0 + tile_w / 3;
    int cx_right = px0 + (2 * tile_w) / 3;
    int cy       = py0 + (2 * tile_h) / 3;

    int r2 = radius * radius;

    // 4. Draw the two filled circles (the cherries)
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            if (dx*dx + dy*dy <= r2) {
                fb_put_pixel(cx_left  + dx, cy + dy, VGA_COL_RED);
                fb_put_pixel(cx_right + dx, cy + dy, VGA_COL_RED);
            }
        }
    }

    // 5. Draw stems (simple green lines up from each cherry)
    int stem_height = radius * 2;
    int stem_top_y  = cy - stem_height;

    if (stem_top_y < py0) stem_top_y = py0;

    // Left stem
    for (int y = cy - radius; y >= stem_top_y; --y) {
        fb_put_pixel(cx_left, y, VGA_COL_GREEN);
    }

    // Right stem
    for (int y = cy - radius; y >= stem_top_y; --y) {
        fb_put_pixel(cx_right, y, VGA_COL_GREEN);
    }

    // 6. Optional: a little connector between stems near the top
    for (int x = cx_left; x <= cx_right; ++x) {
        fb_put_pixel(x, stem_top_y, VGA_COL_GREEN);
    }
}

void DrawGamefield()
{
    //log_vfprintf(VFS_FD_STDOUT, fmt, args);
    for (int y = 0; y < NUM_ROWS; y++) {
        for (int x = 0; x < NUM_COLS; x++) {
            switch (game_window[y][x]){
            case TILE_EMPTY: clear_cell(x, y); break; // empty road
            case TILE_WALL: wall_cell(x, y, VGA_COL_BLUE); break; // wall
            case TILE_DOT_SMALL: dot_cell(x, y, 1); break; // small dot
            case TILE_DOT_BIG: dot_cell(x, y, 3); break; // big dot
            case TILE_GATE: wall_cell(x, y, VGA_COL_GREEN); break; // ghost gate
            case TILE_CHERRY: DrawCherry(x, y); break; // cherry
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