#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* inplemeted in lwt_tramp.S */
extern int * __lwt_trampoline();
extern int * __lwt_trampoline_test();

int main()
{
        __lwt_trampoline_test();
        return 0;
}
