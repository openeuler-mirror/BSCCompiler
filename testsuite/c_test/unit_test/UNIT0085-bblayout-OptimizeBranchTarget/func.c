#define int32_t int
#define uint64_t unsigned long
#define int64_t long

struct S0 {
  int32_t f0;
};
struct {
  struct S0 f4;
} g_88;
uint64_t g_56, func_21_i, g_803 = 1, g_755 = 4073709551615, g_301 = 8;
const volatile int32_t *volatile g_1773 = &g_88.f4.f0;
uint64_t *fn1(int64_t p_22, safe_add_func_uint8_t_u_u, safe_add_func_uint16_t_u_u) {
  int32_t l_1470 = 0;
  for (; 0 < 7; func_21_i++) {
    for (g_803 = 0;; g_803 = safe_add_func_uint8_t_u_u) {
      if (p_22)
        break;
      return &g_56;
    }
    for (l_1470 = 1;; ) {
      if (*g_1773)
        break;
      if (*g_1773) {
        for (g_755 = 2; 0; )
          ;
        continue;
      }
      if (p_22)
        break;
    }
    for (g_301 = 1; 0; g_301 = safe_add_func_uint16_t_u_u)
      ;
  }
}

int main() {
  return 0;
}
