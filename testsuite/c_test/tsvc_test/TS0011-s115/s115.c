#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "common.h"
#include "array_defs.h"

real_t s115(struct args_t * func_args)
{
    initialise_arrays(__func__);
    gettimeofday(&func_args->t1, NULL);
    for (int nl = 0; nl < 1000*(iterations/LEN_2D); nl++) {
        for (int j = 0; j < LEN_2D; j++) {
            for (int i = j+1; i < LEN_2D; i++) {
                a[i] -= aa[j][i] * a[j];
            }
        }
        dummy(a, b, c, d, e, aa, bb, cc, 0.);
    }
    gettimeofday(&func_args->t2, NULL);
    return calc_checksum(__func__);
}

int main(int argc, char ** argv){
    int n1 = 1;
    int n3 = 1;
    int* ip;
    real_t s1,s2;
    init(&ip, &s1, &s2);
    printf("Loop \tTime(sec) \tChecksum\n");

    time_function(&s115, NULL);
    return EXIT_SUCCESS;
}
