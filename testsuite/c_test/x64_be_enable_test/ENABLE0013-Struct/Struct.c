typedef struct d {
  int a;
  int b;
  int c;
  int d;
} IntStruct;

int main() {
  IntStruct data = {6, 9, 3, 4};
  printf("%d", data.b);
  return 0;
}
