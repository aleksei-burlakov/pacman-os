#pragma once
#include "vga.h"

// Tile IDs used in game_window
#define TILE_EMPTY      0
#define TILE_WALL       1
#define TILE_DOT_SMALL  2
#define TILE_DOT_BIG    3
#define TILE_GATE       4
#define TILE_CHERRY     10   // new: cherry

#define CHERRY_SPAWN_X  15
#define CHERRY_SPAWN_Y  17

extern int initial_landscape[NUM_ROWS][NUM_COLS];