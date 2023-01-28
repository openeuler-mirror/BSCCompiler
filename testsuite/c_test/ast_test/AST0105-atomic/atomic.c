// CHECK: [[# FILENUM:]] "{{.*}}/atomic.c"
enum OrderEnum {
  MEMORY_ORDER_RELEAXED = __ATOMIC_RELAXED,
};

char func(int *a){}
int main() {
  int a = 100;
  int b = 89;
  int *prev = &a;
  int *next = &b;
  enum OrderEnum order = MEMORY_ORDER_RELEAXED;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtype i32 C___atomic_store_n
  __atomic_store_n(&prev, (next), order);
  int v = 0;
  int count = 0;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: intrinsiccallwithtypeassigned i32 C___atomic_load_n
  if (__atomic_load_n(&v, order) != count) {
    abort();
  }
  int x;
  __atomic_store_n(&x, 10, 0);
  int y = __atomic_load_n(&x, 0);
  __atomic_add_fetch(&y, 1, 0);
  if (y != 11) {
    abort();
  }
  return 0;
}
