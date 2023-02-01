#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "common.h"
#include "array_defs.h"

real_t s3112(struct args_t * func_args)
{
    initialise_arrays(__func__);
    gettimeofday(&func_args->t1, NULL);
    real_t sum;
    for (int nl = 0; nl < iterations; nl++) {
        sum = (real_t)0.0;
        for (int i = 0; i < LEN_1D; i++) {
            sum += a[i];
            b[i] = sum;
        }
        dummy(a, b, c, d, e, aa, bb, cc, sum);
    }
    gettimeofday(&func_args->t2, NULL);
    return sum;
}

int main(int argc, char ** argv){
    int n1 = 1;
    int n3 = 1;
    int* ip;
    real_t s1,s2;
    init(&ip, &s1, &s2);
    printf("Loop \tTime(sec) \tChecksum\n");

    time_function(&s3112, NULL);
    return EXIT_SUCCESS;
}
