static int g_179 = 0;
static int *volatile g_472 = &g_179;
inline static void func() {
  int *l_1117 = &g_179;
  *l_1117 = 2778;  // this will change the volatile value (*g_472),
                   // do not delete it.
  *l_1117 = *g_472;
}

int main() {
  func();
  if (g_179 != 2778) {
    abort();
  }
}

