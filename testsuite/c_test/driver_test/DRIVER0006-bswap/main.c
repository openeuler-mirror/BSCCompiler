#include <stdint.h>
#include <stdio.h>

int main(void) {
    uint64_t data64 = 0x012345678abcdef4;
    printf("%lx\n", data64);
    uint64_t res64 = __builtin_bswap64(data64);
    printf("%lx\n", res64);

    uint32_t data32 = 0x012345678;
    printf("%x\n", data32);
    uint32_t res32 = __builtin_bswap32(data32);
    printf("%x\n", res32);

    uint16_t data16 = 0x1234;
    printf("%hx\n", data16);
    uint16_t res16 = __builtin_bswap16(data16);
    printf("%hx\n", res16);
}