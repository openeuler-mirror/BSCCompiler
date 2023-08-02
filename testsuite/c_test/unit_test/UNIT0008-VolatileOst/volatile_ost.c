// For fixing `compilation error` in SimplifyCFG from DTS2021120818280
// Description:
//   maple middle end won't optimize volatile variable, so the following
//   `return x` has no phi node for x. We skip cond2select in SimplifyCFG
//   when phi is empty to cover the issue
#define GUARD 8

int main (void) {
  volatile int x;
  x = 8;

  if ( GUARD >= x ) {
     x = 0;
  } else {
     x = 1;
  }

  return x;
}

