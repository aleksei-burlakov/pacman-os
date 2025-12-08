#include "header_panel.h"
#include "vga.h"

static void Update1UP(char* text)
{
    draw_text(2, 1, text, VGA_COL_WHITE);
}

uint32_t g_high_score = 0;

static void UpdateHighScore(char* text)
{
    draw_text(12, 0, "HIGH SCORE", VGA_COL_YELLOW);
    draw_text(14, 1, text, VGA_COL_WHITE);
}

void Score1UP(int score)
{
    // Simple clamping (optional, but avoids negative weirdness)
    if (score < 0) score = 0;

    char buf[12];  // enough for 32-bit decimal + '\0'
    u32_to_str_dec((uint32_t)score, buf);
    Update1UP(buf);

    if(g_high_score < score) {
        g_high_score = score;
        UpdateHighScore(buf);
    }
}

void Reset1UP()
{
    Update1UP("00");
}

void InitializeTopPanel()
{
    draw_text(1, 0, "1 UP", VGA_COL_YELLOW);
    //fb_rectfill(0, 0, FB_WIDTH-1, TOP_OFFSET * TILE_H - 1, VGA_COL_BLACK);
    Reset1UP();
    UpdateHighScore("00");
    //draw_text(2, 0, "HIGH SCORE  10000", VGA_COL_YELLOW);
    //draw_text(2, 1, "1UP    00", VGA_COL_WHITE);
}