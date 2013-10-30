#include <stdio.h>

void
foo()
{
	__asm__ __volatile__(	"pushal\n"
							"popal\n"
							"movl $11223, %%eax\n"
							:
							:
							:"%eax"
						);
}

int
main()
{
	int a=10;
	foo();
	a++;
	printf("%d\n", a);
	return 0;
}