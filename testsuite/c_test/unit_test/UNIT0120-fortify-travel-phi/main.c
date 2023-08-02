#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>

int decfile(FILE *fin, FILE *fout, char* ifn, char* ofn)
{   char    inbuf1[16], inbuf2[16], outbuf[16], *bp1, *bp2, *tp;
    int     i, l, flen;
    while(1)
    {
        i = fread(bp1, 1, 16, fin);
        for(i = 0; i < 16; ++i)
            outbuf[i] ^= bp2[i];
        l = i; tp = bp1, bp1 = bp2, bp2 = tp;
    }
    return 0;
}

int main() {
  return 0;
}
