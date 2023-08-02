struct a {
  int x;
  int y;
};

const struct b {
  int x;
  struct a y_0[10][10];
  struct a y_1[10][10][10];
  int z;
} a = {
  .x = 1,
  .z = 2
};

int main() {
  if (a.z != 2 || a.x != 1) {
    foo();
  }
  return 0;
}
