__thread int a = 10;
int foo(int b) {
    a = b + 5;
    return 0;
}
