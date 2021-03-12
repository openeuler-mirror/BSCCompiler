#include "typedef.h"
extern int64 fooi64(int64 a ,int64 b);
extern int64 addi64I(int64 a);

int main() {
  if (fooi64(0x1234343, 0x123456789) == (0x1234343+0x123456789))
    printf("%llx pass\n", fooi64(0x1234343, 0x123456789));
  else
    printf("failed\n");
  if (addi64I(0x1234343) == (0x1234343+0x123456789))
    printf("pass\n");
  else {
    printf("failed\n");
    printf("%llx vs 0x12468aacc\n", addi64I(0x1234343));
  }
  if (addi64I(0x800000008) == (0x800000008 + 0x800000008))
    printf("pass3\n");
  else {
    printf("failed3\n");
    printf("%llx\n", addi64I(0x800000008));
  }
}
