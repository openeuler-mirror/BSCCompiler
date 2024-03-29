#include "stdlib.h"
#include "stdio.h"
#define ARRAY_SIZE 500000
#define NLOOPS1 5
#define NLOOPS2 200

// RNG implemented localy to avoid library incongruences
#ifdef RAND_MAX
#undef RAND_MAX
#endif
#define RAND_MAX 32767
static unsigned long long int next = 1;
int rand( void ) {
    next = next * 1103515245 + 12345;
    return (unsigned int)(next / 65536) % RAND_MAX+1;
}
void srand( unsigned int seed ) {
    next = seed;
}
// End of RNG implementation

int randInt(int min, int max) {
    int k, n;
    n = (max - min) + 1;
    k = (int)(n * (rand() / (RAND_MAX + 1.0)));
    return (k == n) ? k + min - 1 : k + min;
}

void shuffle(int* items, int len) {
    size_t j, k, i;
    int aux;

    for(i = len-1; i > 0; --i) {
        k = (int)((i + 1) * (rand() / (RAND_MAX + 1.0)));
        j = (k == (i + 1)) ? k - 1 : k;

        aux = items[i];
        items[i] = items[j];
        items[j] = aux;
    }
}

int *createRandomArray(int size) {
    int i, len;
    int *result;

    len = size + 1;
    //result = (int*)malloc(0);
    result = (int*)malloc(len * sizeof(int));
    for (i = 0; i < len; i++)
    // CHECK-NOT: [[# @LINE ]] warning: cant't prove the offset < the upper bounds
    // CHECK-NOT: [[# @LINE ]] warning: cant't prove the offset >= the lower bounds
        result[i] = i;
    result[0] = randInt(1, size); // CHECK: [[# @LINE ]] warning: can't prove the pointer < the upper bounds when accessing the memory
    shuffle(result, len);
    return result;
}

int findDuplicate(int *data, int len) {
    int i;
    int result = 0;

    for (i = 0; i < len; i++)
        result = result ^ (i + 1) ^ data[i];
    result ^= len;
    return result;
}

int main() {
    int i, j, duplicate;
    int *rndArr;

    srand(1);

    for (i = 0; i < NLOOPS1; i++) {
        rndArr = createRandomArray(ARRAY_SIZE);
        for (j = 0; j < NLOOPS2; j++)
            duplicate = findDuplicate(rndArr, ARRAY_SIZE+1);
        free(rndArr);
        printf("Found duplicate: %d\n", duplicate);
    }

    return 0;
}
