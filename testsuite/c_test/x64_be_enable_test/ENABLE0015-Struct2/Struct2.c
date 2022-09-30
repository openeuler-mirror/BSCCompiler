typedef struct IntStruct_t {
  int a;
  int b;
} IntStruct;

int main() {
  IntStruct data = {1, 2};
  IntStruct* ptr = &data;
  ptr->b = 4;
  printf("%d,%d\n", ptr->a, ptr->b);
  return 0;
}