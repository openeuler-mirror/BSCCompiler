#pragma pack(push)
#pragma pack(1)
struct S2 {
   unsigned f0 : 6;
} __attribute__((aligned(16)));
#pragma pack(pop)

char a = 1;
struct S2 b = {2};

int foo(char a, struct S2 b) {
    return 0;
}

int main() {
    // CHECK:  ldr x1,{{.*}}
    // CHECK-NEXT: ldr x2,{{.*}}
    // CHECK-NEXT: bl foo
    int c = foo(a, b);
    return 0;
}
