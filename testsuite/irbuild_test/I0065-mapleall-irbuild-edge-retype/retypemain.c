extern int retypef32toi32(float x);
extern float retypei32tof32(int x);
extern long long int retypef64toi64(double x);
extern double retypei64tof64(long long int x);

int main(void) {
    union {float f1; int i1;} valf1, valf2;
    union {double d1; long long int i1;} vald1, vald2;
    valf1.f1 = 1.2343;
    valf2.f1 = 22.222;
    vald1.d1 = 1111111111111.1234343434;
    vald2.d1 = 222222222222222222.22222222222222;
    printf("%d vs %d\n", valf1.i1, retypef32toi32(valf1.f1));
    printf("%f vs %f\n", valf1.f1, retypei32tof32(valf1.i1));
    printf("%lld vs %lld\n", vald1.i1, retypef64toi64(vald1.d1));
    printf("%f vs %f\n", vald1.d1,  retypei64tof64(vald1.i1));
    return 0;
}

