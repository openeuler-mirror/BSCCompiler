int a[10];

int main() {
  int i = 0;
  a[0] = 1;
  // can not compute trip count
  while ((double) 8.584 >= i) {
    a[i]++;
    i++;
  }
  if (a[0] != 2) {
    abort();
  }
  return 0;
}
