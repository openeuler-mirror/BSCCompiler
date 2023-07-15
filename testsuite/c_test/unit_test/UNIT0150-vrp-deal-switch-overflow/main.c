#include "stdint.h"

int64_t a = -765048523176745064LL;
int64_t b = -922337203685477580;
int64_t c = 922337203685477580;

void f163(void) {
    switch (a) {
        case 9223372036854775806:
            ++c;
            break;
        case 0:
            --b;
            break;
        default:
            break;
    }
}

int main() {
    f163();
    return 0;
}
