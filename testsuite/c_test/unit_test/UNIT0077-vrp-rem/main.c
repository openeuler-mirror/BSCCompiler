#include <limits.h>
#include <stdio.h>
#include "stdint.h"
extern void abort ();

int main()
{
   static uint8_t x533 = 10U;
   int32_t x534 = 300;
   int32_t x535 = -4486;
   int32_t t105 = -1;
   int32_t x536 = -128;
   t105 = (((x533-x534)%x535)%x536);

   if (t105 != -34) { abort(); } else { ; }
   return 0;
}
