enum OrderEnum {
  MEMORY_ORDER_RELEAXED = __ATOMIC_RELAXED,
};
int main() {
  int a = 100;
  int b = 89;
  int *prev = &a;
  int *next = &b;
  enum OrderEnum order = MEMORY_ORDER_RELEAXED;
  __atomic_store_n(&prev, (next), order);
  return 0;
}
