#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>

int __lwt_start()
{
	printf("lwt start!\n");
}

int __lwt_start_test()
{
	__lwt_start();
}
