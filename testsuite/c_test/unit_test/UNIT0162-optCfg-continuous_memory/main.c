struct A {
  void *smac;
  void *dmac;
};

int main() {
  struct A *pkt;
  if ((*((int *) (((pkt)->smac))) == 0x0 && *((short *) (((char *) (((pkt)->smac))) + 4)) == 0x0) ||
     (*((int *) (((pkt)->dmac))) == 0x0 && *((short *) (((char *) (((pkt)->dmac))) + 4)) == 0x0)) {
    return 0;
  }
  return 0;
}
