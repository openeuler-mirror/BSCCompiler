char a[] = {255};
int main() {
  char *b = &a[0];
  a[0] && ++*b;
  printf("%d\n", a[0]);
  return 0;
}
