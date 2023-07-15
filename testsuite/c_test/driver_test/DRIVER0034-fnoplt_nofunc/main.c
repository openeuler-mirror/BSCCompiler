struct a {
  char b;
} __attribute__((aligned(128))) c;

struct a d() {
  return c;
}

int main() {}