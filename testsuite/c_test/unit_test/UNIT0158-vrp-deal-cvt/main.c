#include "stdio.h"
#define max(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })
#define min(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })
int main() {
  printf("left - right = %llu\n", ((unsigned long long int)(/*left*/(min((1648961877U),
         (((unsigned int)(unsigned char)8)))) - /*right*/(((unsigned int)min(
         (min((-1698283966), (((/* implicit */ int)(_Bool)1)))),
         (((/* implicit */int)(unsigned char)90))))))));
  return 0;
}
