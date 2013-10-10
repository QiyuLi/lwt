#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>

/* inplemeted in lwt_tramp.S */
//extern int * __lwt_trampoline();
//extern int * __lwt_trampoline_test();

int main()
{
	void *x= (void *) 7;
	__asm__ __volatile__ ("sub $0x4,%esp");
//	__asm__ __volatile__ ("movl $0x7,(%esp)");
	__asm__ __volatile__ ("movl %0, (%%esp)"
				: 
				: "r"(x)	);
        __asm__ __volatile__ ("call __lwt_trampoline_test");
        return 0;
}
