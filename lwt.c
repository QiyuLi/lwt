#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stddef.h>

#include <lwt.h>


#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

volatile unsigned long long startx, endx;

/**

 Gets the offset of a field inside a struc

*/
#define LWT_STRUCT_OFFSET(Field) \
	[Field] "i" (offsetof(struct __lwt_t__, Field))


/* Global Variables */

int counter = 0;

lwt_tcb *tcb[MAX_THREAD_SIZE];
void *tcb_index;
void *stack_index;

lwt_tcb *
__get_tcb(lwt_t lwt)
{
	return tcb[lwt];
}

lwt_tcb *curr_thd;
lwt_tcb *queue_head = NULL;
lwt_tcb *queue_tail = NULL;


/* run queue functions */

int 
__queue_remove(lwt_tcb *thd)
{
	if(!thd)
		return -1;

	thd->prev->next = thd->next;

	if(thd->next)
		thd->next->prev = thd->prev;
	else
		queue_tail = thd->prev;


	thd->next = thd->prev = NULL;


	return 0;
}

int 
__queue_add(lwt_tcb *thd)
{
	if(!thd)
		return -1;
	
	if(queue_tail){
	    	queue_tail->next = thd;
		thd->prev = queue_tail;	
	}
	else{
	    	queue_head->next = thd; 
		thd->prev = queue_head;
	}

	queue_tail = thd;

	return 0;
}

void 
__queue_print(int i)
{	
	lwt_tcb *temp; 

	
	lwt_tcb *prev = curr_thd;

	if(i>-1){
		curr_thd = tcb[i];

		__queue_remove(tcb[i]);
		__queue_add(prev);
	}

	if(curr_thd)
	   	printf("curr %d \n",curr_thd->tid);

	for(temp = queue_head->next; temp; temp = temp->next){   
	    	printf("%d ",temp->tid);
	}

	printf("\n");
	
	for(temp = queue_tail; temp != queue_head; temp = temp->prev){   
	    	printf("%d ",temp->tid);
	}

	printf("\n");
}

/* lwt functions */


lwt_t 
lwt_create(lwt_fn_t fn, void *data)
{

	int temp = counter;

	if(!temp){
		tcb_index   = malloc(sizeof(lwt_tcb) * MAX_THREAD_SIZE);
		stack_index = malloc(DEFAULT_STACK_SIZE * MAX_THREAD_SIZE);
		queue_head  = malloc(sizeof(lwt_tcb));

		tcb[temp]         = tcb_index;
		tcb[temp]->tid    = temp;
		tcb[temp]->status = RUNNABLE;
		
		curr_thd = tcb[temp];
		
		temp++;
		counter++;

		//printf("lwt create: add main %x\n",tcb[temp]->status);
	}

	tcb[temp]         = tcb_index + sizeof(lwt_tcb) * temp;
	tcb[temp]->stack  = stack_index + DEFAULT_STACK_SIZE * temp;
	tcb[temp]->tid    = temp;
	tcb[temp]->status = READY;	
	tcb[temp]->bp     = tcb[temp]->stack + DEFAULT_STACK_SIZE - 0x4 ;
	tcb[temp]->fn     = fn;
	tcb[temp]->data   = data;

	counter++;

	__queue_add(tcb[temp]);

	return temp;

}

int 
lwt_yield(lwt_t lwt)
{
//unsigned long long start2, end2;

//rdtscll(start2);	
	if(lwt == LWT_NULL){
		__lwt_schedule();

//rdtscll(end2);
//printf("Overhead  of lwt_yield is %lld \n", (end2-start2));
        	return 0;
	}

	lwt_tcb *prev_thd = curr_thd;
	curr_thd = tcb[lwt];

	__queue_remove(curr_thd);

	__queue_add(prev_thd);

	__lwt_dispatch(curr_thd,prev_thd);



	return 0;
}

lwt_t 
lwt_current(void)
{
        return curr_thd->tid;
}

int 
lwt_id(lwt_t lwt)
{
        return lwt;
}

void *
lwt_join(lwt_t lwt)
{
//unsigned long long start1, end1;

	lwt_tcb *thd = tcb[lwt];

	int i = 0;

	while(thd->status != FINISHED){
		i++;
//rdtscll(start1);
		//printf("lwt join while: tid %x status %x \n",thd->tid,thd->status);
		lwt_yield(LWT_NULL);
//rdtscll(end1);
//printf("Overhead  of lwt_join is %lld %d\n", (end1-start1),i);
		
	}
	void * retVal = thd->retVal;

	//if(!thd->stack)
	//	free(thd->stack);
	//free(thd);

        return (void *) retVal;
}

void 
lwt_die(void *val)
{
	curr_thd->status = FINISHED;
	curr_thd->retVal = val;	

	//printf("lwt_die\n");
}

/* private lwt functions */


void 
__lwt_schedule(void)
{
//unsigned long long start5, end5;

	if(!queue_head || !queue_head->next)
		return;

	lwt_tcb *prev_thd = curr_thd;


	curr_thd = queue_head->next;
	
	if(prev_thd->status != FINISHED)
		__queue_add(prev_thd);

	__queue_remove(curr_thd);
	

//rdtscll(start5);
	__lwt_dispatch(curr_thd,prev_thd);
//rdtscll(end5);

//printf("Overhead  of lwt_schedule is %lld \n", (end5-start5));

}

__attribute__ ((noinline))
void 
__lwt_dispatch(lwt_tcb *next, lwt_tcb *curr)
{



	//printf("lwt_dispatch: next %x %x %x\n", next->tid, next->bp, next->sp);
	//printf("lwt_dispatch: curr %x %x %x\n", curr->tid, curr->bp, curr->sp);

	//assert(next->fn);

	__asm__ __volatile__("push %2\n"
			     "pushal\n"
			     "movl %%esp, %0\n"
			     "movl %%ebp, %1\n"
			     :"=r"(curr->sp),"=r"(curr->bp)
			     :"r"(&&return_here)
                            );	
	

	if(next->status == READY){
		next->status = RUNNABLE;
	    	__lwt_initial(next);		
	}
	else{	
		__asm__ __volatile__("mov %0,%%esp\n"
				     "mov %1,%%ebp\n"
				     "popal\n"
				     "ret\n"
                             	     :
			             :"r"(next->sp), "r"(next->bp)
                                     : 
                            	    );
	}
	

return_here:
	return;
}

__attribute__ ((noinline))
void 
__lwt_initial(lwt_tcb *thd) 
{
	//assert(thd);
	//printf("__lwt_initial: %x %x %d\n", thd->bp, thd->fn, thd->data);



	__asm__ __volatile__("movl %0,%%esp\n\t"
                             :
			     :"r"(thd->bp - 8)
                             :
                             );


	__asm__ __volatile__("movl %0,(%%esp)\n\t"
                             :
			     :"r"(thd->fn)
                             :
                             );
//rdtscll(startx);
	__asm__ __volatile__("movl %0,0x4(%%esp)\n\t"
                             :
			     :"r"(thd->data)
                             :
                             );

//rdtscll(endx);

	__asm__ __volatile__("call __lwt_trampoline_test\n\t"			
                             );

	//__lwt_trampoline();
}

__attribute__ ((noinline))
void 
__lwt_start(lwt_fn_t fn, void * data)
{

//printf("Overhead  of disptach + init + tramp is %lld \n", (endx-startx));


	//assert(fn > 0x80000);
       	//printf("__lwt_start: %x %d\n", fn, data);	

	void * retVal = fn(data);

	//printf("__lwt_start end\n");
	//printf("__lwt_start: retval %d \n", retVal);

	lwt_die(retVal);



	lwt_yield(LWT_NULL);
}

int
lwt_info(lwt_info_t t)
{
	switch( t ) 
	{
		case LWT_INFO_NTHD_RUNNABLE:
       			return -1;
    		case LWT_INFO_NTHD_ZOMBIES:
       			return -1;
		case LWT_INFO_NTHD_BLOCKED:
       			return -1;
    		default :
        		return -1;
	}	
}
lwt_tcb *
__get_thread(lwt_t lwt)
{
	return tcb[lwt];
}


