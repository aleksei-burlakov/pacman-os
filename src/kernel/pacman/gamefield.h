#pragma once
#include "vga.h"

extern int game_window[NUM_ROWS][NUM_COLS];
extern uint32_t g_tick;

extern struct Actor {
    int pos_y;
    int pos_x;

    // We keep the last position to prefer going forward
    // rather that back
    int last_pos_y;
    int last_pos_x;
    uint8_t color;
} ghost5, ghost6, ghost7, ghost8, pacman;

void DrawGamefield();