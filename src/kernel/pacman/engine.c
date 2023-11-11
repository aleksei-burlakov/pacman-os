#include "engine.h"
#include <arch/i686/isr.h>
#include <debug.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_ROWS                24
#define NUM_COLS                28
#define VGA_BLACK               0
#define VGA_BLUE                1
#define VGA_GREEN               2
#define VGA_CYAN                3
#define VGA_RED                 4
#define VGA_MAGENTA             5
#define VGA_YELLOW              14
#define VGA_GREEN_BACKGROUND    VGA_GREEN << 4
#define VGA_RED_BACKGROUND      VGA_RED << 4
#define VGA_BLACK_SQUARE        0
#define VGA_WHITE_SQUARE        255

#define MODULE  "PACMAN"
#define IRQ0_PERIOD             11  // trigger timer every 15th tick

struct Actor {
    int pos_y;
    int pos_x;

    // We keep the last position to prefer going forward
    // rather that back
    int last_pos_y;
    int last_pos_x;
    uint8_t color;
    unsigned char symbol;
} ghost5, ghost6, ghost7, ghost8, pacman;

int initial_landscape[NUM_ROWS][NUM_COLS] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 3, 1, 0, 0, 1, 2, 1, 0, 0, 0, 1, 2, 1, 1, 2, 1, 0, 0, 0, 1, 2, 1, 0, 0, 1, 3, 1 },
    //{ 1, 2, 1, 0, 0, 1, 2, 1, 0, 0, 0, 1, 2, 1, 1, 2, 1, 0, 0, 0, 1, 2, 1, 0, 0, 1, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    //{ 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1 },
    //{ 0, 0, 0, 0, 0, 1, 2, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 1, 1, 1, 4, 4, 1, 1, 1, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 0, 0, 0, 6, 5, 0, 1, 0, 0, 0, 2, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0, 8, 7, 0, 0, 0, 1, 0, 1, 1, 2, 0, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1 },
    { 0, 0, 0, 0, 0, 1, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 1, 0, 0, 0, 0, 0 },
    { 1, 1, 1, 1, 1, 1, 2, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 2, 1, 1, 1, 1, 1, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    //{ 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 2, 1 },
    { 1, 3, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 9, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 3, 1 },
    //{ 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1 },
    { 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 2, 1, 1, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1 },
    { 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};

int game_window[NUM_ROWS][NUM_COLS];
int Rand4();

void DrawActor(struct Actor ghost)
{
    // TODO: check collisions
    VGA_putchr(ghost.pos_x, ghost.pos_y, ghost.symbol);
    VGA_putcolor(ghost.pos_x, ghost.pos_y, ghost.color);
}

void DrawWindow()
{
    //vfprintf(VFS_FD_STDOUT, fmt, args);
    for (int y = 0; y < NUM_ROWS; y++) {
        for (int x = 0; x < NUM_COLS; x++) {
            switch ( game_window[y][x] ) {
            case 0: VGA_putchr(x, y, ' '); VGA_putcolor(x, y, VGA_BLACK_SQUARE); break; // black path
            case 1: VGA_putchr(x, y, ' '); VGA_putcolor(x, y, VGA_WHITE_SQUARE); break; // white wall
            case 2: VGA_putchr(x, y, '.'); break;
            case 3: VGA_putchr(x, y, '*'); break;
            case 4: VGA_putchr(x, y, ' '); VGA_putcolor(x, y, VGA_GREEN_BACKGROUND); break; // grean exit
            default: VGA_putchr(x, y, ' '); VGA_putcolor(x, y, VGA_BLACK_SQUARE); break;
            }
        }
    }
    DrawActor(ghost5);
    //DrawActor(ghost6);
    //DrawActor(ghost7);
    //DrawActor(ghost8);
    DrawActor(pacman);
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
    if (ghost->last_pos_y != ghost->pos_y) {
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
    }
    while (false == mooved) {
        int direction = Rand4(); // 0..3
        switch(direction) {
        case 0: // move down
            if (game_window[ghost->pos_y+1][ghost->pos_x] != 1) {
                ghost->last_pos_y = ghost->pos_y;
                ghost->pos_y++;
                mooved = true;
            }
            break;
        case 1: // move up
            if (game_window[ghost->pos_y-1][ghost->pos_x] != 1) {
                ghost->last_pos_y = ghost->pos_y;
                ghost->pos_y--;
                mooved = true;
            }
            break;
        case 2: // move right
            if (game_window[ghost->pos_y][ghost->pos_x+1] != 1) {
                ghost->last_pos_x = ghost->pos_x;
                ghost->pos_x++;
                mooved = true;
            }
            break;
        case 3: // move left
            if (game_window[ghost->pos_y][ghost->pos_x-1] != 1) {
                ghost->last_pos_x = ghost->pos_x;
                ghost->pos_x--;
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

// Rocket-scince true-random generator between [0 and 3]
int Rand4()
{
    static int round = 0;
    round++;
    switch(round) {
    case 1: return 1;
    case 2: return 3;
    case 3: return 0;
    case 4: return 2;
    case 5: return 1;
    case 6: return 0;
    case 7: return 2;
    case 8: round = 0;
        return 3;
    }
}

void MainLoop()
{
    for (int i = 0; i < 500; i++) {
        log_debug("PACMAN", "Ok, we are in the main loop");
        DrawWindow();
        MoveGhost(&ghost5);
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

void Initialize()
{
    // 1. Setup the timer
    i686_IRQ_RegisterHandler(0, irq0_handler_timer);
    
    // 2. Initialize ghosts
    for (int y = 0; y < NUM_ROWS; y++) {
        for (int x = 0; x < NUM_COLS; x++) {
            switch ( initial_landscape[y][x] ) {
            case 5: ghost5.pos_y = y; ghost5.last_pos_y = y; ghost5.pos_x = x; ghost5.last_pos_x = x; ghost5.color = VGA_RED; ghost5.symbol = 'G'; break;
            case 6: ghost6.pos_y = y; ghost6.last_pos_y = y; ghost6.pos_x = x; ghost6.last_pos_x = x; ghost6.color = VGA_CYAN; ghost6.symbol = 'G'; break;
            case 7: ghost7.pos_y = y; ghost7.last_pos_y = y; ghost7.pos_x = x; ghost7.last_pos_x = x; ghost7.color = VGA_MAGENTA; ghost7.symbol = 'G'; break;
            case 8: ghost8.pos_y = y; ghost8.last_pos_y = y; ghost8.pos_x = x; ghost8.last_pos_x = x; ghost8.color = VGA_YELLOW; ghost8.symbol = 'G'; break;
            case 9: pacman.pos_y = y; pacman.last_pos_y = y; pacman.pos_x = x; pacman.last_pos_x = x; pacman.color = VGA_YELLOW; pacman.symbol = 'C'; break;
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