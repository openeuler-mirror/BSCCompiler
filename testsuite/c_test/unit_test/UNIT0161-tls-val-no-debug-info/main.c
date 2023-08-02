__thread int a = 0;
int main() {
    a++;
    printf("no tls val loc emitted.\n");
    return 0;
}
// CHECK-NOT: R_AARCH64_ABS64 used with TLS symbol
