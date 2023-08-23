#include <stdint.h>

__attribute__((noinline)) uint32_t safe_lshift_func_uint32_t_u_s(uint32_t left,
                                                                 int right) {
  return ((((int)right) < 0) || (((int)right) >= 32) ||
          (left > ((4294967295U) >> ((int)right))))
             ? ((left))
             : (left << ((int)right));
}

__attribute__((noinline)) int32_t safe_rshift_func_int32_t_s_s(int32_t left,
                                                               int right) {
  return ((left < 0) || (((int)right) < 0) || (((int)right) >= 32))
             ? ((left))
             : (left >> ((int)right));
}

uint16_t g_158 = 0x0EB3L;

int main() {
  int g_90 =
      safe_lshift_func_uint32_t_u_s(
          safe_rshift_func_int32_t_s_s((~g_158) ^ 0x75FBD47FL, 1), 18) >= 0L;
  printf("g_90 = %x\n", g_90);
  return 0;
}
