#include <stdio.h>
#include <stdlib.h>

#define ARG 13
#define EXPECT 26

int func(int input)
{
    int result = input;
    int *a = &input;
    asm volatile ("ldr x1, [%[a]]\n\t"
                "add %[result], %[result], x1\n\t"
                : [result] "+X" (result)
                : [a] "p" (a)
                : "x1");
    return result;
}

int main()
{
    int result = func(ARG);
    if (EXPECT != result) {
        printf("Error: expect result is %d, but actual result is %d\n", EXPECT, result);
        return 1;
    }
    return 0;
}
