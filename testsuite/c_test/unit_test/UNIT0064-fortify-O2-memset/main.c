/*
 *- @TestCaseID: fortify_source_13.c
 *- @TestCaseName: fortify_source_13
 *- @TestCaseType: Function test
 *- @RequirementID: AR.SR.IREQf4722263.001.006
 *- @RequirementName: -D_FORTIFY_SOURCE=2
 *- @Design Description: buffer overflow not occurs and don't call __builtin_chk funtion
 *- @Condition: Lit test environment ready
 *- @Brief:
 *   -#step1 compile it
 *   -#step2 check result
 *- @Expect: PASS
 *- @Priority: Level 1
 */

#include <stdio.h>
#include <string.h>

#define LEN 10

int main()
{
    char dest[LEN] = "11111";
    int err = 0;

    memset(dest, 'A', 0);
    if(dest[0] != '1') {
        err--;
    }
    memset(dest, 'B', LEN);
    if(dest[ LEN - 1] != 'B') {
        err++;
    }
    return err;
}


// RUN: %CC %CFLAGS   %FORTIFY_2 %s -o %t
// RUN: %SIM %t

