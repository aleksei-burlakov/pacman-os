#pragma once
#include <stdint.h>

static inline int my_abs(int v) {
    return (v < 0) ? -v : v;
}

// Convert unsigned integer to decimal string. `buf` must be large enough.
// Returns `buf` for convenience.
char* u32_to_str_dec(uint32_t value, char* buf);