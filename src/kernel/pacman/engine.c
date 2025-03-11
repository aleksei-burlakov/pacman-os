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

// BEGIN: copied from i8259.c
#define PIC1_COMMAND_PORT           0x20
#define PIC_CMD_END_OF_INTERRUPT    0x20
// END: copied from i8259.c

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

// VGA 16-color palette (0-15)
enum VGA_COLORS {
    BLACK = 0x0, BLUE = 0x1, GREEN = 0x2, CYAN = 0x3,
    RED = 0x4, MAGENTA = 0x5, BROWN = 0x6, LIGHT_GRAY = 0x7,
    DARK_GRAY = 0x8, LIGHT_BLUE = 0x9, LIGHT_GREEN = 0xA, LIGHT_CYAN = 0xB,
    LIGHT_RED = 0xC, LIGHT_MAGENTA = 0xD, YELLOW = 0xE, WHITE = 0xF
};

// Function to put a pixel in VGA 640x480 mode (Mode 0x12)
void put_pixel(int x, int y, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    // Calculate memory offset: (y * 320) + (x / 2)
    uint16_t offset = (y * 320) + (x / 2);

    // Read the existing byte from memory
    uint8_t current_byte = VGA_ADDRESS[offset];

    if (x % 2 == 0) {
        // Left pixel (high nibble, upper 4 bits)
        current_byte = (current_byte & 0x0F) | (color << 4);
    } else {
        // Right pixel (low nibble, lower 4 bits)
        current_byte = (current_byte & 0xF0) | (color & 0x0F);
    }

    // Store the modified byte back into video memory
    VGA_ADDRESS[offset] = current_byte;
}

void MainLoop()
{
    for (int i = 0; i < 3; i++) {
        log_debug("PACMAN", "Ok, we are in the main loop");
        DrawWindow();
        MoveGhost(&ghost5);
        Wait();
    }

    for (int i = 0; i < 20; i++) {
        put_pixel(100, 50+i, 1);   // Draw white pixel at (100, 50)
        put_pixel(101, 50+i, 2);   // Draw white pixel at (100, 50)
        put_pixel(102+i, 51, 3); // Draw red pixel next to it
        put_pixel(102+i, 52, 4); // Draw red pixel next to it
        //Wait();
    }

    put_pixel(100, 50, WHITE);       // White
    put_pixel(101, 50, LIGHT_RED);   // Red
    put_pixel(102, 50, GREEN);       // Green
    put_pixel(103, 50, BLUE);        // Blue
    put_pixel(104, 50, YELLOW);      // Yellow
    put_pixel(105, 50, MAGENTA);     // Magenta
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