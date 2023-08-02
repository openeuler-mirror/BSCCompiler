enum color { red, green, blue };

static void break_me(void) {

}

static void call_me(enum color e) {
  break_me();
}

int main() {
  call_me(green);
  return 0;
}
