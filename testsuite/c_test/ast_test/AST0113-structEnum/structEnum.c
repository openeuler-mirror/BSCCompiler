enum Color {
  RED,
  GREEN,
  BLUE,
};

typedef int X;

struct Point {
  enum Color color;
  X x;
} point;

int main() {
  point.color = GREEN;
  point.x = 2;
  return 0;
}
