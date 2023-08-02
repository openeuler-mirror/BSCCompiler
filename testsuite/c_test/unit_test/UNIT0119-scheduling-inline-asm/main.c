#include <stdio.h>

int func()
{
    register int result asm("x3");
    asm volatile ("mov x2, %[a]\n\t"
                "sub x2, x2, %[a]\n\t"
                "orr x3, x2, %[a]\n\t"
                :
                : [a] "I" (1)
                : "x2");
    return result;
}

int main()
{
    int result = func();
    printf("%d\n", result);
    return 0;
}

