#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

#include "common.h"
#include "array_defs.h"

real_t s174(struct args_t * func_args)
{
    int M = *(int*)func_args->arg_info;
    initialise_arrays(__func__);
    gettimeofday(&func_args->t1, NULL);
    for (int nl = 0; nl < 10*iterations; nl++) {
        for (int i = 0; i < M; i++) {
            a[i+M] = a[i] + b[i];
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

    time_function(&s174, &(struct{int a;}){LEN_1D/2});
    return EXIT_SUCCESS;
}