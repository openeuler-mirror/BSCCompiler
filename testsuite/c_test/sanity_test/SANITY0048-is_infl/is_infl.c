#include <stdio.h>
#include <float.h>

#define  ARG0  (-1.0)
#define  ARG1  1.0

int TESTDouble(double x, int expV)
{
    int res = __builtin_isinf_sign (x);
    if (res != expV) {
        printf("Error: expect result is %d, but actual result is %d.\n", expV, res);
        return 1;
    }
    return 0;
}

int TESTFloat(float x, int expV)
{
    int res = __builtin_isinf_sign (x);
    if (res != expV) {
        printf("Error: expect result is %d, but actual result is %d.\n", expV, res);
        return 1;
    }
    return 0;
}

int TESTLongDouble(long double x, int expV)
{
    int res = __builtin_isinf_sign (x);
    if (res != expV) {
        printf("Error: expect result is %d, but actual result is %d.\n", expV, res);
        return 1;
    }
    return 0;
}

int main()
{
    int ret = 0;
    ret += TESTDouble((double) (ARG0 / 0.0), -1);
    ret += TESTDouble((double) (ARG1 / 0.0), 1);
    ret += TESTFloat((float) (ARG0 / 0.0), -1);
    ret += TESTFloat((float) (ARG1 / 0.0), 1);
    ret += TESTLongDouble((long double) (ARG0 / 0.0), -1);
    ret += TESTLongDouble((long double) (ARG1 / 0.0), 1);
    return ret;
}