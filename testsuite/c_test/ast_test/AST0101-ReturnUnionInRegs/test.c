typedef union {
    double el1[2];
    double el2;
} test_union1_t;

typedef union {
    double el1[2];
    char el2;
} test_union2_t;

test_union1_t g_union1;
test_union2_t g_union2;

test_union1_t func1(void) {
    // CHECK: ld{{[rp]}}     d{{[0-1]+}}
    return g_union1;
}

test_union2_t func2(void) {
    // CHECK: ldr     x{{[0-1]+}}
    return g_union2;
}

int main(void) {
    test_union1_t loc1 = func1();
    test_union2_t loc2 = func2();
}
