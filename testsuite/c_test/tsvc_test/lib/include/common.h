#ifndef TSVC_COMMON_HDR
#define TSVC_COMMON_HDR

#define iterations 1000
#define LEN_1D 3200
#define LEN_2D 256
#define ARRAY_ALIGNMENT 64

#include <sys/time.h>

struct args_t {
    struct timeval t1;
    struct timeval t2;
    void * __restrict__ arg_info;
};

#if 0
typedef double real_t;
#define ABS fabs
#else
typedef float real_t;
typedef real_t(*test_function_t)(struct args_t *);
#define ABS fabsf
#endif

int dummy(real_t[LEN_1D], real_t[LEN_1D], real_t[LEN_1D], real_t[LEN_1D], real_t[LEN_1D], real_t[LEN_2D][LEN_2D], real_t[LEN_2D][LEN_2D], real_t[LEN_2D][LEN_2D], real_t);

void init(int** ip, real_t* s1, real_t* s2);

int initialise_arrays(const char* name);
real_t calc_checksum(const char * name);
void time_function(test_function_t vector_func, void * arg_info);

#endif
