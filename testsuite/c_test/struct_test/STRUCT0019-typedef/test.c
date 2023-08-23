typedef struct A {
    int a;
    char b[129];
    int c;
} B __attribute__ ((aligned(128)));

typedef struct C {
    int a;
    char b[129];
    int c;
}D __attribute__ ((aligned(128)));

struct __attribute__ ((aligned(128))) E {
    int a;
    char b[129];
    int c;
};

struct A Aa[10];
B Bb[10];
struct C Cc[10];
D Dd[10];
struct E Ee[10];

int main() {
    printf("Aa size = %d\n", sizeof(Aa));
    printf("Bb size = %d\n", sizeof(Bb));
    printf("Cc size = %d\n", sizeof(Cc));
    printf("Dd size = %d\n", sizeof(Dd));
    printf("Ee size = %d\n", sizeof(Ee));
    return 0;
}