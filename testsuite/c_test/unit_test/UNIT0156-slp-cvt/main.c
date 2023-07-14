typedef int i32;
typedef unsigned long long u64;

__attribute__((noinline))
void foo(i32 * __restrict__ dst, u64 x) {
  // When tree node type (u64 here) is different from tree type (i32 here),
  // SLP should add extra cvt if needed.
  dst[0] &= x;
  dst[1] &= x;
  dst[2] &= x;
  dst[3] &= x;
}

int main() {
  i32 arr[] = { 1, 2, 3, 4 }; 
  foo(arr, 42);
  return 0;
}

