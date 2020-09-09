

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static uint8_t ramWr[1024/8];

#define HP41CXno

int main() {
int i;
    // system
    for (i = 0x000; i <= 0x00F; i++)            // 16
        ramWr[i>>3] |= 1 << (i & 0x07);
    // ram only 41cv
    for (i = 0x0C0; i <= 0x1FF; i++)            // + 320
        ramWr[i>>3] |= 1 << (i & 0x07);

#ifdef HP41CX
    // xmemory
    for (i = 0x040; i <= 0x0BF; i++)            // + 128
        ramWr[i>>3] |= 1 << (i & 0x07);
    for (i = 0x201; i <= 0x2EF; i++)            // + 239
        ramWr[i>>3] |= 1 << (i & 0x07);
    for (i = 0x301; i <= 0x3EF; i++)            // + 239
        ramWr[i>>3] |= 1 << (i & 0x07);
#endif

    for (i=0; i < sizeof(ramWr);) {
        printf("0x%02X, ", ramWr[i++]);
        if ((i % 16) == 0) {
            printf("\n");
        }
    }
    return 0;
}

