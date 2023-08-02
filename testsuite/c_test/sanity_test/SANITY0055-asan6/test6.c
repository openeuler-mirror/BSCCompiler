// ./run_ubsan.sh test6.c
int main() {
    int mas[11][12][13];
    mas[9][12][1] = 100;
    return 0;
}
