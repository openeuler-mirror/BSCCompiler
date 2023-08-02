#include <stdio.h>

int main(){

    char a;
    a = 'a';
    printf("%c\n", a);

    unsigned char b;
    b = 'b';
    printf("%c\n", b);

    signed char c;
    c = 'c';
    printf("%c\n", c);

    int d;
    d = 1;
    printf("%d\n", d);

    unsigned int e;
    e = 2;
    printf("%d\n", e);

    short f;
    f = 3;
    printf("%d\n", f);

    unsigned short g;
    g = 4;
    printf("%d\n", g);

    long h;
    h = 5;
    printf("%ld\n", h);

    float i;
    i = 1.;
    printf("%f\n", i);

    double j;
    j = 0.7623e-2;
    printf("%lf\n", j);

//    long double k;
//    k = 0.7623e-2;
//    printf("%Lf\n", k);

    long long l;
    l = 100;
    printf("%lld\n", l);

    return 0;
}
