extern int ceilf64toi32(double x);
extern int ceilf32toi32(float x);
extern long long int ceilf64toi64(double x);
extern long long int ceilf32toi64(float x);

int main(void) {
    printf("%d\n", ceilf64toi32(1.22));
    printf("%d\n", ceilf64toi32(1.00f));
    printf("%d\n", ceilf64toi32(0));
    printf("%d\n", ceilf64toi32(-1));
    printf("%d\n", ceilf64toi32(-1.22));
    printf("%d\n", ceilf64toi32(-111.22));
 
    printf("%d\n", ceilf32toi32(1.22));
    printf("%d\n", ceilf32toi32(1.00f));
    printf("%d\n", ceilf32toi32(0));
    printf("%d\n", ceilf32toi32(-1));
    printf("%d\n", ceilf32toi32(-1.22));
    printf("%d\n", ceilf32toi32(-111.22));

    printf("%lld\n", ceilf64toi64(111111111111.21));
    printf("%lld\n", ceilf64toi64(1.22));
    printf("%lld\n", ceilf64toi64(1.00f));
    printf("%lld\n", ceilf64toi64(0));
    printf("%lld\n", ceilf64toi64(-1));
    printf("%lld\n", ceilf64toi64(-1.22));
    printf("%lld\n", ceilf64toi64(-111.22));
    printf("%lld\n", ceilf32toi64(1.22));
    printf("%lld\n", ceilf32toi64(1.00f));
    printf("%lld\n", ceilf32toi64(0));
    printf("%lld\n", ceilf32toi64(-1));
    printf("%lld\n", ceilf32toi64(-1.22));
    printf("%lld\n", ceilf32toi64(-111.22));
      return 0;
}

