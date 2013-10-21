#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>


void * joo(void *data)
{
	printf("joo: %d\n", (int)data);

	//lwt_yield(NULL);

	printf("joo: after yield \n");

	return NULL;
}

void * goo(void *data)
{
	printf("goo: %d\n", (int)data);

	//lwt_yield(NULL);

	printf("goo: after yield \n");

	return NULL;
}

void * bar(void *data)
{
	printf("bar: %d\n", (int)data);
	
	//lwt_yield(lwt_NULL);

	int i;
	__asm__ __volatile__("movl %%ebp, %0\n\t"
                             :"=r"(i)
			     :
                             :
                             );
	
	printf("bar: after yield %x\n",i);

	return (int)data * 10;
}

void * foo(void *data)
{
	printf("foo: %d\n", (int)data);

	//lwt_yield(0);

	int i;
	__asm__ __volatile__("movl %%ebp, %0\n\t"
                             :"=r"(i)
			     :
                             :
                             );

	printf("foo: after yield %x\n",i);

	return (int)data * 10;
}

int main()
{
	printf("main: \n");

	lwt_fn_t fn = foo;
	void *data = (void *) 5;
	lwt_t thd1 = lwt_create(fn,data);
	

	fn = bar;
	data = (void *) 10;
	lwt_t thd2 = lwt_create(fn,data);

/*


	fn = goo;
	data = (void *) 15;
	lwt_t thd3 = lwt_create(fn,data);

*/
	//__lwt_initial(thd1);
	//__lwt_initial(thd2);
	//__run_queue_print();
	//int i = __run_queue_remove(thd3);
	//__run_queue_print();

	lwt_tcb * temp = __get_tcb(thd1);

	printf("main: stack %x \n", temp->stack);
	
	//free(temp->stack);


	lwt_yield(thd1);


	void *retVal = lwt_join(thd1);
	printf("main: thd1 rt %d \n", (int)retVal);

	retVal = lwt_join(thd2);
	printf("main: thd2 rt %d \n", (int)retVal);


        return 0;
}
