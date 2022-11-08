#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define MALLOC(n, type) ((type *) malloc((n)* sizeof(type)))
#define LEN 5

static char dest[LEN - 1] = "111";

static void vsnprintf_base(int arg, ...)
{
    char* str;
    if(rand()) {
        str = MALLOC(LEN + 1, char);
        str = "abcde";
    } else {
        str = MALLOC(LEN, char);
        str = "abcd";
    }
    va_list args;
    va_start(args, arg);
    vsnprintf(dest, strlen(str), str, args);
    va_end(args);
}

int main()
{
    int res;
    int err = 0;

    vsnprintf_base(res);
    res = memcmp(dest, "abcd", LEN - 1);
    if(res != 0) {
        err++;
    }
    return err;
}


// RUN: %CC %CFLAGS   %FORTIFY_2 %s -o %t
// RUN: %OBJDUMP -S %t | FileCheck %s
// CHECK: __vsnprintf_chk
// RUN: ! %SIM %t
