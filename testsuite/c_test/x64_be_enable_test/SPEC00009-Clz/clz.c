#include <stdio.h>
int main() {
    unsigned a = 14;
    printf("%d\n", __builtin_clz(a));

    unsigned long b = 24;
    printf("%d\n", __builtin_clzl(b));

    unsigned long long c = 235;
    printf("%d\n", __builtin_clzll(c));
}
