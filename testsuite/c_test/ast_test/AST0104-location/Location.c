int multi_line_if_conditional (int a, int b, int c) {
  if (a  /* set breakpoint 3 here */
      && b
      && c)
    return 0;
  else
    return 1;
}

int main() {
  int a = 2;
  int b = 3;
  int c = 4;

  return multi_line_if_conditional(a, b, c);
}
