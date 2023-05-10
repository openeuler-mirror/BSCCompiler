#include <stdarg.h>
#include <stdio.h>

long double funct2(int NumArgs, ...)
{
    long double AddVal = 0.0;
    va_list vl;

    va_start(vl, NumArgs);
    while (NumArgs--) {
        AddVal += va_arg(vl, long double);
    }

    va_end(vl);

    return AddVal;
}

int main()
{
    long double d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, expd, gotd;
    d0 = 0.1e+1;
    d1 = 1.02e+1;
    d2 = 1.003e+1;
    d3 = 1.0004e+1;
    d4 = 1.00005e+1;
    d5 = 1.000006e+1;
    d6 = 1.0000007e+1;
    d7 = 1.00000008e+1;
    d8 = 1.000000009e+1;
    d9 = 1.0000000000e+1;
    expd =  d0 + d1 + d2 + d3 + d4 + d5 + d6 + d7 + d8 + d9;
    gotd = funct2(10,d0,d1,d2,d3,d4,d5,d6,d7,d8,d9);

    if (gotd != expd) {
        printf("%Le, expect %Le\n", gotd, expd);
        return 1;
    }

    return 0;
}
