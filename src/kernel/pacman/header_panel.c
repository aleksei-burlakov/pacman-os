#include "header_panel.h"
#include "vga.h"

void Update1UP(char* text)
{
    draw_text(1, 0, "1 UP", VGA_COL_YELLOW);
    draw_text(2, 1, text, VGA_COL_WHITE);
}

void UpdateHighScore(char* text)
{
    draw_text(10, 0, "HIGH SCORE", VGA_COL_YELLOW);
    draw_text(10, 1, text, VGA_COL_WHITE);
}

void InitializeTopPanel()
{
    //fb_rectfill(0, 0, FB_WIDTH-1, TOP_OFFSET * TILE_H - 1, VGA_COL_BLACK);
    Update1UP("00");
    UpdateHighScore("00");
    //draw_text(2, 0, "HIGH SCORE  10000", VGA_COL_YELLOW);
    //draw_text(2, 1, "1UP    00", VGA_COL_WHITE);
}