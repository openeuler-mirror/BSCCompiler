// ./run_ubsan.sh test6.c
int main() {
    int mas[11][12][13];
    // mas[11][9] = 20;
    // mas[9][10][13] = 100;
    mas[9][12][1] = 100;
}
