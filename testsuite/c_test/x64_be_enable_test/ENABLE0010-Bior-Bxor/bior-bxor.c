#include "stdio.h"
int main() {
	int a = 1;
	int b = 2;
	int m = a | b;
	printf("%d\n", m);
	int x = a ^ b;
	printf("%d\n", x);
	return 0;
}
