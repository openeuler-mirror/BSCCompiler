#include <stdio.h>
typedef unsigned __int128 uint128;
typedef __int128 int128;

int main() {
    // CHECK: [[# @LINE + 1]] PTY_u128 is not fully supported
    uint128 u128 = 1;
    // CHECK: [[# @LINE + 1]] PTY_i128 is not fully supported
    int128 i128 = 1;

    printf("%llx\n", (unsigned long long)(u128 & 0xFFFFFFFFFFFFFFFF));
    printf("%llx\n", (long long)(i128));
    return 0;
}