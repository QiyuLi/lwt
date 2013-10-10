#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>

/* Global Variables */
lwt_tcb *tcb[MAX_THREAD_SIZE];
lwt_t current=0;

int prestart = 0;
static void __lwt_prestart()
{
	for(prestart = 0; prestart < MAX_THREAD_SIZE; prestart++)
	{
		tcb[prestart] = NULL;
	}
	return;
}

/* lwt functions */
int counter = 1;
lwt_t lwt_create(lwt_fn_t fn, void *data)
{
	return NULL;
}

void *lwt_join(lwt_t lwt)
{
        return;
}

void lwt_die(void *val)
{
        return;
}

int lwt_yield(lwt_t lwt)
{
        return NULL;
}

lwt_t lwt_current(void)
{
        return NULL;
}

int lwt_id(lwt_t lwt)
{
        return NULL;
}

/* private lwt functions */
void __lwt_schedule(void)
{
        return;
}

void __lwt_dispatch(lwt_t next, lwt_t current)
{
        return;
}

void __lwt_start()
{
        return;
}

void __lwt_start_test(void *data)
{
	int a = (int) data;
	printf("lwt start! %d\n", a);
}
