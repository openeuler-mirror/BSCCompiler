#include <stdio.h>
int Add(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j) {
    int res = a + b + c + d + e + f + g + h + i + j;
    int m = g;
    printf("%d\n", m);
    return res;
}
int main() {
    int a = 1;
    int b = 2;
    int c = 3;
    int d = 4;
    int e = 5;
    int f = 6;
    int g = 7;
    int h = 8;
    int i = 9;
    int j = 10;
    int res = Add(a, b, c, d, e, f, g, h, i ,j);
    printf("%d\n", res);
    return 0;
}

