#define max(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a > _b ? _a : _b; })
#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
       _a < _b ? _a : _b; })

short arr[23][23];
int main() {
  for (int i_0 = 0; i_0 < 23; ++i_0)
    for (int i_1 = 0; i_1 < 23; ++i_1)
       arr[i_0] [i_1] = (short)-28427;
   
  int x = min((2021076666U), (arr[0][0]));
  int y = min(0,  (long long int)arr[0][0]);
  int var_113 = min(x, y);
  printf("%d\n", var_113);
}
