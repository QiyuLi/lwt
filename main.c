#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>

/* inplemeted in lwt_tramp.S */
//extern int * __lwt_trampoline();
//extern int * __lwt_trampoline_test();

int main()
{
        __asm__ __volatile__ ("call __lwt_trampoline_test");
        return 0;
}
