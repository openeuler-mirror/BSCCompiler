#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#define LEN 6

int main()
{
    char dest[LEN] = "11111";
    char *str = "abcde";
    int res;
    int err = 0;
    char *cp;

    cp = mempcpy(dest, str, 0);//mempcpy返回指针指向复制到dest后的字符串后一位，在此处由于长度为0指向dest本身
    if(dest[0] != '1' || cp != dest) {
        printf("cp =%p, dest = %p\n", cp, dest);
        err++;
    }

    cp = mempcpy(dest, str, LEN);//在此处长度为LEN指向dest + LEN
    res = memcmp(dest, str, LEN);
    if(res != 0 || cp != dest + LEN) {
        printf("cp =%p, dest = %p\n", cp, dest + LEN);
        err++;
    }
    return err;
}
