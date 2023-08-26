#include <stdio.h>

void abort(void);

__thread int t0;
__thread int t1 = 66;


int main(int argc, char **argv)
{
    printf("t0 = %d\n", t0);
    printf("t1 = %d\n", t1);
    return 0;
}
