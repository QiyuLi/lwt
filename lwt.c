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
int counter = 0; //TID self increase counter

lwt_t tcb[MAX_THREAD_SIZE];
void *tcb_index = NULL;
void *stack_index = NULL;

lwt_t curr_thd = NULL;

void *tramp_addr = NULL;


/* run queue functions */
lwt_t queue_head = NULL;
lwt_t queue_tail = NULL;

int 
__queue_remove(lwt_t thd)
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
	int tid = -1;
	tid = counter++;
	lwt_t t = NULL;

	if(!tid){
		// Initialize memory
		tcb_index   = malloc(sizeof(lwt_tcb) * MAX_THREAD_SIZE);
		stack_index = malloc(DEFAULT_STACK_SIZE * MAX_THREAD_SIZE);
		queue_head  = malloc(sizeof(lwt_tcb));

		// Initialize tramp address
		__asm__ __volatile__("LEAL __lwt_initial, %0"
							:"=r" (tramp_addr)
							);

		// Assign main thread tcb
		t = tcb_index;
		tcb[tid] = t;
		t->tid    = tid;
		t->status = LWT_ACTIVE;
		
		curr_thd = t;
		//printf("lwt create: add main %x\n",tcb[tid]->status);
	}

	tid=counter++;

	// Assign child thread
	t = tcb_index + sizeof(lwt_tcb) * tid;
	tcb[tid] = t;
	t->stack  = stack_index + DEFAULT_STACK_SIZE * tid;
	t->tid    = tid;
	t->status = LWT_RUNNABLE;	
	t->bp     = t->stack + DEFAULT_STACK_SIZE - 0x4 ;
	t->ip		= tramp_addr;
	t->fn     = fn;
	t->data   = data;

	__queue_add(t);

	return t;

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

	lwt_t prev_thd = curr_thd;
	curr_thd = lwt;

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
        return lwt->tid;
}

void *
lwt_join(lwt_t lwt)
{
//unsigned long long start1, end1;

	lwt_t thd = lwt;

	int i = 0;

	while(thd->status != LWT_ZOMBIES){
		i++;
//rdtscll(start1);
		//printf("lwt join while: tid %x status %x \n",thd->tid,thd->status);
		lwt_yield(LWT_NULL);
//rdtscll(end1);
//printf("Overhead  of lwt_join is %lld %d\n", (end1-start1),i);
		
	}
	void *retVal = thd->retVal;

	//if(!thd->stack)
	//	free(thd->stack);
	//free(thd);

	return retVal;
}

void 
lwt_die(void *val)
{
	curr_thd->status = LWT_ZOMBIES;
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

	lwt_t prev_thd = curr_thd;


	curr_thd = queue_head->next;
	
	if(prev_thd->status != LWT_ZOMBIES)
		__queue_add(prev_thd);

	__queue_remove(curr_thd);
	

//rdtscll(start5);
	__lwt_dispatch(curr_thd,prev_thd);
//rdtscll(end5);

//printf("Overhead  of lwt_schedule is %lld \n", (end5-start5));

}

void 
__lwt_dispatch(lwt_t next, lwt_t curr)
{
	//printf("lwt_dispatch: next %x %x %x\n", next->tid, next->bp, next->sp);
	//printf("lwt_dispatch: curr %x %x %x\n", curr->tid, curr->bp, curr->sp);

	//assert(next->fn);

	__asm__ __volatile__(	"pushal\n"
							"push $1f\n"
							"movl %%ebp, %0\n"
							"movl %%esp, %1\n"
							"movl %2,%%esp\n"
							"movl %3,%%ebp\n"
							"ret\n"
							"1:"
							"popal\n"
							:"=r"(curr->bp),"=r"(curr->sp)
							:"r"(next->sp), "r"(next->bp)
						);
	return;
}

void 
__lwt_initial() 
{
	//assert(thd);
	//printf("__lwt_initial: %x %x %d\n", thd->bp, thd->fn, thd->data);
	__asm__ __volatile__(	"push %0\n"
							"push %1\n"
							"call __lwt_trampoline_test\n"
							:
							:"r" (curr_thd->data), "r" (curr_thd->fn)
						);
}

void 
__lwt_start(lwt_fn_t fn, void * data)
{

//printf("Overhead  of disptach + init + tramp is %lld \n", (endx-startx));


	//assert(fn > 0x80000);
       	//printf("__lwt_start: %x %d\n", fn, data);	

	void *retVal = fn(data);

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

lwt_t
__get_thread(lwt_t lwt)
{
	return lwt;
}


