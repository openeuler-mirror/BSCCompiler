int g = 0;

// "localZero / a" is a loop invariant, but do not
// move it out of the loop, because it may throw exception
// and kills the whole program.
__attribute__ ((noinline))
void testDivByZeroInLoop(int a, int count) {
  int localZero = 0;
  for (int i = 0; i < count; i++) {
    g += (a ? a : (localZero / a) ? 0 : 1); // In fact, it's "localZero / 0".
  }
}

int main() {
  testDivByZeroInLoop(5, 6);
  return 0;
}

