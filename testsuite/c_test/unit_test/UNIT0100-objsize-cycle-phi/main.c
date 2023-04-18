struct TbmTblOpStat {
  int a;
  int b;
};

struct TbmRoot {
  int tbmOpCnt[100];
  int tbmOpFailCnt[100];
};

int func(struct TbmRoot *tbmRoot, num) {
  for (int i = 0; i < num; ++i) {
    memset(&tbmRoot->tbmOpCnt[i], 0, 10);
    memset(&tbmRoot->tbmOpFailCnt[i], 0, 10);
  }
  return 0;
}

int main() {
  return 0;
}
