#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MALLOC(n, type) ((type *) malloc((n)* sizeof(type)))
#define LEN 5

int main(int argc, char * argv[])
{
    char dest[LEN - 3];
    char *str;
    int res;
    int err = 0;

    if(argc > 1) {
        str = MALLOC(LEN, char);
        str = "abcd";
    } else {
        str = MALLOC(LEN + 1, char);
        str = "abcde";
    }

    sprintf(dest, "%s", str);
    res = memcmp(dest, str, LEN);
    if(res != 0) {
        err++;
    }
    return err;
}

//测试在打开fortify_source下已知destnation，但在运行时才能确定需要写入的副本大小时，应该调用检查函数在运行发生溢出时报错
// RUN: %CC %CFLAGS   %FORTIFY_2 %s -o %t
// RUN: %OBJDUMP -S %t | FileCheck %s
// CHECK: __sprintf_chk
// RUN: ! %SIM %t

