#include "stdint.h"
#include "stdio.h"

void _cdecl cstart_(uint16_t bootDrive)
{
    puts("Hello world from C!\n");
    printf("Formatted %% %c %s\r\n", 'a', "string");
    printf("Formatted %d %i %x %p %o %hd %hi %hhu %hhd\r\n", 1234, -5678, 0xdeed, 0xbeef, 012345, (short)27, (short)-42, (unsigned char)20, (char)-10);
    printf("Formatted %ld %lx %lld %llx\r\n", -100000000l, 0xdeedbeeful, 10200300400ll, 0xdeedbeeffeeddeedull);
    for(;;);
}
