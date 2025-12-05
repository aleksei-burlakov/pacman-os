#include "gamefield.h"
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

void DrawGamefield()
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