#pragma once
#include <stdio.h>
#include <arch/i686/io.h>
#include <arch/i686/irq.h>


void StartGame();

static inline int my_abs(int v) {
    return (v < 0) ? -v : v;
}
