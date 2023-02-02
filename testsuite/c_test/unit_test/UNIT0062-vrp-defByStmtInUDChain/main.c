#include <stdio.h>      
static int g_5 = 0;
static int g_6 = 0x0488F4F3L;
static int g_10 = 0xB16EB2D3L;
inline static int  func_1(void) {
    {
         for (g_5 = 6;(g_5 >= 0);g_5 -= 1) {
            for (g_6 = 0; ;g_6 += 1)             {
               for (g_10 = 9;(g_10 >= 2);g_10 -= 1);
               break;
            }
         }
    }
}
int main (int argc, char* argv[]) {
    func_1();
    printf ("%d\n", g_10);
}
