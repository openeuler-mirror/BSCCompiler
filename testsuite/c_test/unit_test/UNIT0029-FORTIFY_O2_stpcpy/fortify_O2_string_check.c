#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#define LEN 6

int main()
{
    char dest[LEN];
    char *str = "abcde";
    int res;
    char *cp = dest;
    int err = 0;

    cp = stpcpy(dest, str);//stpcpy预期返回值为指向dest结尾'\0'的指针
    res = memcmp(dest, str, LEN);
    if(res != 0 || cp != &dest[LEN - 1]) {
        printf("cp = %p, dest[5] = %p", cp , &dest[5]);
        err++;
    }
    return err;
}
