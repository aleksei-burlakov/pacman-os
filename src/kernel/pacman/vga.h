#pragma once
#include <stdint.h>

// Размеры нашего виртуального фреймбуфера
#define FB_WIDTH   640
#define FB_HEIGHT  480

#define HEADER_TILES   2  // top pane
#define FOOTER_TILES   2  // bottome pane
#define TOP_OFFSET     HEADER_TILES

#define NUM_ROWS                29
#define NUM_COLS                28

#define TILE_W     (FB_WIDTH  / NUM_COLS)   // 640 / 28
#define TILE_H     (FB_HEIGHT / (HEADER_TILES + NUM_ROWS + FOOTER_TILES)) // 480px / (2+29+2)

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

// Фреймбуфер в ОЗУ (объявление, сам массив будет в vga.c)
extern uint8_t g_framebuffer[FB_WIDTH * FB_HEIGHT];

// Базовые примитивы (то, что сейчас в engine.c как static)
void fb_put_pixel(int x, int y, uint8_t color);
void fb_clear(uint8_t color);
void fb_rectfill(int x0, int y0, int x1, int y1, uint8_t color);
void fb_dot(int cx, int cy, uint8_t color, int radius);
void fb_circle(int cx, int cy, int r, uint8_t color);
void fb_line(int x0, int y0, int x1, int y1, uint8_t color);

// Можно оставить, если нужен
void vga_blit_framebuffer_12h(void);

// Текст (если уже есть fb_draw_char)
void fb_draw_char(int x, int y, char c, uint8_t color);
void fb_draw_text(int x, int y, const char* s, uint8_t color);
