
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
 
int main(void)
{
	long double ld = 10000.2222L;
	foo();
	puts("hello world!???(y/n)");
	printf("long double type size is %lu %lu\n",sizeof(long double),sizeof ld);
	return 0;
}
