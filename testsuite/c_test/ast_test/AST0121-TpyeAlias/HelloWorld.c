typedef int foo;
foo i = 33;
enum Num {
	ONE,
	TWO,
};
enum Num b = ONE;

struct A {
  int a;
  foo b;
};
struct A a = {1, 1};

int main() {
  if (i >2) {
    typedef float foo;
    struct A {
      int c;
      foo d;
    };
    foo f = 1.8;
    struct A a = {1, 1.0};
		enum Num {
			THREE,
		};
    i += (int)f;
		enum Num b = THREE;
  } else {
    typedef char foo;
    foo c = 8;
		enum Num {
			FOUR,
		};
		enum Num b = FOUR;
    i += c;
  }
	printf("%d\n", i);
  return 0;
}

int test () {
	struct AA {
		int a;
	};
	
}
