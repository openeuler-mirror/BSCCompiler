#include <stdio.h>
int main () {
        int d = 13;
        int *a = &d;
        int b = *a;
        int c = b;
        printf("%d\n", c);
        return 0;
}
