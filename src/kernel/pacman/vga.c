#include "vga.h"
#include <arch/i686/io.h>  //для i686_outb

#define VGA_MEM ((uint8_t*)0xA0000)

static inline void vga_set_map_mask(uint8_t mask)
{
    i686_outb(0x3C4, 0x02);  // Sequencer index 2 = Map Mask
    i686_outb(0x3C5, mask);
}

uint8_t g_framebuffer[FB_WIDTH * FB_HEIGHT];

void fb_put_pixel(int x, int y, uint8_t color)
{
    if (x < 0 || x >= FB_WIDTH || y < 0 || y >= FB_HEIGHT) return;
    g_framebuffer[y * FB_WIDTH + x] = color & 0x0F;  // 4-bit color
}

void fb_clear(uint8_t color)
{
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            fb_put_pixel(x, y, color);
}

void fb_rectfill(int x0, int y0, int x1, int y1, uint8_t color)
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

inline void fb_dot(int cx, int cy, uint8_t color, int radius)
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

void fb_circle(int cx, int cy, int r, uint8_t color)
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

void fb_line(int x0, int y0, int x1, int y1, uint8_t color)
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


static void vga_draw_char_7x14_px(int px, int py, char c, uint8_t color)
{
    uint8_t ch = (uint8_t)c;
    const uint8_t *glyph = &fontdata_7x14[ch * FONT_HEIGHT];

    fb_rectfill(px, py, px + TILE_W, py + TILE_H, VGA_COL_BLACK);

    for (int row = 0; row < FONT_HEIGHT; row++) {
        uint8_t bits = glyph[row];   // одна строка = 1 байт

        for (int col = 0; col < FONT_WIDTH; col++) {
            // используем биты 7..1 (как в комментариях к шрифту)
            int bit_index = 7 - col; // 7,6,5,4,3,2,1
            if (bits & (1u << bit_index)) {
                fb_put_pixel(px + col, py + row, color);
            }
        }
    }
}


void draw_text(int tile_x, int tile_y, const char* s, uint8_t color)
{
    // базовая точка по X — левый край тайла
    int px = tile_x * TILE_W;

    // по Y — ВНУТРИ своей панели, без TOP_OFFSET
    int py = tile_y * TILE_H + (TILE_H - FONT_HEIGHT) / 2;

    // 4 пикселя слева и 4 справа. В итоге 2 символа на тайл
    const int CHAR_SPACING = 4;

    while (*s) {
        vga_draw_char_7x14_px(CHAR_SPACING /*слева*/ + px, py, *s, color); // then draw the symbol
        px += FONT_WIDTH + /*справа*/ CHAR_SPACING;
        s++;
    }
}


