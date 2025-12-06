#include "header_panel.h"
#include "vga.h"

static void Update1UP(char* text)
{
    draw_text(2, 1, text, VGA_COL_WHITE);
}

static void UpdateHighScore(char* text)
{
    draw_text(12, 0, "HIGH SCORE", VGA_COL_YELLOW);
    draw_text(14, 1, text, VGA_COL_WHITE);
}

void Score1UP(int score)
{
    // STOPPED HERE: Implement itoa
    switch(score) {
    case 1: Update1UP("1"); break;
    case 2: Update1UP("2"); break;
    case 3: Update1UP("3"); break;
    case 4: Update1UP("4"); break;
    case 5: Update1UP("5"); break;
    case 6: Update1UP("6"); break;
    case 7: Update1UP("7"); break;
    case 8: Update1UP("8"); break;
    case 9: Update1UP("9"); break;
    case 10: Update1UP("10"); break;
    default: Update1UP("INFINITY"); break;
    }
}

void Reset1UP()
{
    Update1UP("00");
}

void InitializeTopPanel()
{
    draw_text(1, 0, "1 UP", VGA_COL_YELLOW);
    Reset1UP();
    UpdateHighScore("00");
    //draw_text(2, 0, "HIGH SCORE  10000", VGA_COL_YELLOW);
    //draw_text(2, 1, "1UP    00", VGA_COL_WHITE);
}