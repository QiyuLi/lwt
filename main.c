#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>

/* inplemeted in lwt_tramp.S */
//extern int * __lwt_trampoline();
//extern int * __lwt_trampoline_test();

void *t123(void *d) __attribute__ ((noinline));

void *t123(void *d)
{
	printf("T123 In t 123\n");
}

int main()
{
//	printf("main before t123\n");
	lwt_fn_t x = t123;
//	printf("%d\n", (int) t123);
//	printf("main afer t123\n");
//	void *x= (void *) t123;
//	printf("%d\n", (int) t123);
	__asm__ __volatile__ ("sub $0x4,%esp");
	__asm__ __volatile__ ("movl %0, (%%esp)"
				: 
				: "r"(x)	);
        __asm__ __volatile__ ("call __lwt_trampoline_test");
        return 0;
}
