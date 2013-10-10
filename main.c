#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>

/* inplemeted in lwt_tramp.S */
//extern int * __lwt_trampoline();
//extern int * __lwt_trampoline_test();

int main()
{
	int x=7;
	__asm__ __volatile__ ("sub $0x4,%esp");
	__asm__ __volatile__ ("movl $0x7,(%esp)");
//	__asm__ __volatile__ ("push %0"
//				:
//				: "r" (x)	);
        __asm__ __volatile__ ("call __lwt_trampoline_test");
        return 0;
}
