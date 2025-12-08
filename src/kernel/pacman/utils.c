#include "utils.h"

char* u32_to_str_dec(uint32_t value, char* buf)
{
    // max 32-bit unsigned = 4294967295 → 10 digits + '\0'
    char tmp[11];
    int i = 0;

    if (value == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return buf;
    }

    // Build reversed decimal digits in `tmp`
    while (value > 0 && i < 10) {
        uint32_t digit = value % 10;
        value /= 10;
        tmp[i++] = (char)('0' + digit);
    }

    // Reverse into output buffer
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';

    return buf;
}