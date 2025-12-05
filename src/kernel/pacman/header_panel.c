#include "header_panel.h"
#include "vga.h"

void DrawTopPanel()
{
    // --- верхняя панель ---
    fb_rectfill(0, 0, FB_WIDTH-1, TOP_OFFSET * TILE_H - 1, VGA_COL_BLACK);

    fb_draw_text(1, 0, "SCORE: 123", VGA_COL_WHITE);
    fb_draw_text(20, 0, "LIVES: 3", VGA_COL_WHITE);
}