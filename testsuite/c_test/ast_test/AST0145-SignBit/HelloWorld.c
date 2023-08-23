// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"
#include <math.h>

int main()
{
    float fn = -1.0;
    double dn = -11.00;
    // CHECK: LOC {{.*}} 10 10
    // CHECK:  callassigned &__signbitf (dread f32 %fn_6_11) { dassign %retVar_323 0 }
    if ((__builtin_signbit(fn)) == 0) {
        printf("error\n");
    }
    // CHECK: LOC {{.*}} 15 10
    // CHECK:  callassigned &__signbitf (cvt f32 f64 (dread f64 %dn_7_12)) { dassign %retVar_327 0 }
    if ((__builtin_signbitf(dn)) == 0) {
        printf("error\n");
    }
    return 0;
}