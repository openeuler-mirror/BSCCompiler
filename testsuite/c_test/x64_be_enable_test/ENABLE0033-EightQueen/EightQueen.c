#define QUEEN_NUM (8)

int queen[QUEEN_NUM] = {0};
int num = 0;

int abs(int n) {
  return n > 0 ? n : -n;
}

void show() {
  printf("%d:", ++num);
  for (int i = 0; i < QUEEN_NUM; ++i) {
    printf("(%d,%d) ", i, queen[i]);
  }
  printf("\n");
}

int check(int n) {
  for (int i = 0; i < n; ++i) {
    if (queen[i] == queen[n] || abs(queen[n] - queen[i]) == abs(n - i)) {
      return 1;
    }
  }
  return 0;
}

void eight_queen(int n) {
  for (int i = 0; i < QUEEN_NUM; ++i) {
    queen[n] = i;
    if (check(n) == 0) {
      if (n == QUEEN_NUM - 1) {
        show();
      } else {
        eight_queen(n + 1);
      }
    }
  }
}

int main() {
  eight_queen(0);
  return 0;
}