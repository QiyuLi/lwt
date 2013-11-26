#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <lwt.h>
#include <lwt_chan.h>
#include <queue.h>

//Performance test

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

volatile unsigned long long startx, endx;

//rdtscll(startx);
	
//rdtscll(endx);

//printf("Overhead of lwt_schedule is %lld \n", (endx-startx));


/* Global Variables */

int thd_count = 0;
int initialized = 0;

lwt_t tcb[MAX_THREAD_SIZE];

void *tcb_index;
void *stack_index;


queue *run_queue;
queue *wait_queue;

lwt_t curr_thd;


/* lwt functions */

int
lwt_info(lwt_info_t t)
{
	int i;
	int c[6] = {0,0,0,0,0,0,0};

	for(i = 0; i < MAX_THREAD_SIZE; i++)
		if(tcb[i]) c[tcb[i]->state]++;

	return c[t];
}

lwt_t 
lwt_current(void)
{
	if(curr_thd)
        	return curr_thd;
	else{
		__lwt_initialize();	
		return curr_thd;
	}
}

int 
lwt_id(lwt_t lwt)
{
        return lwt->tid;
}

lwt_t 
lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flag, void *c)
{
	if(!initialized){
		__lwt_initialize();
	}

	if(thd_count == MAX_THREAD_SIZE)
		return NULL;

	int temp;

	for(temp = 1; temp < MAX_THREAD_SIZE; temp++)
		if(!tcb[temp] || tcb[temp]->state == LWT_INFO_NTHD_FINISHED)
			break;
	
	//printf("lwt create temp = %d, data  = %x\n",temp, data);

	if(!tcb[temp]){
		tcb[temp] = tcb_index + sizeof(lwt_tcb) * temp;
		tcb[temp]->stack  = stack_index + DEFAULT_STACK_SIZE * temp;
	}

	tcb[temp]->tid      = temp;
	tcb[temp]->state    = LWT_INFO_NTHD_NEW;	
	tcb[temp]->bp       = tcb[temp]->stack + DEFAULT_STACK_SIZE - 0x4 ;
	tcb[temp]->fn       = fn;
	tcb[temp]->data     = data;
	tcb[temp]->joinable = flag;
	tcb[temp]->parent_thd   = lwt_current();
	tcb[temp]->self_node = q_make_node(tcb[temp]);
	tcb[temp]->group    = NULL;

	if(c)	
		((lwt_chan_t)c)->rcv_thd = tcb[temp];

	tcb[temp]->chan     = c;

	thd_count++;

	q_enqueue(run_queue, tcb[temp]->self_node);

	return tcb[temp];
}

int 
lwt_yield(lwt_t lwt)
{
	//printf("lwt_yield %d %d \n",wait_queue->size, run_queue->size);

	if(!initialized){
		__lwt_initialize();
	}

	if(lwt == LWT_NULL)
		__lwt_schedule(run_queue, NULL);
	else
		__lwt_schedule(run_queue, lwt);

	return 0;
}



void *
lwt_join(lwt_t lwt)
{		
	//lwt_tcb *curr = curr_thd;

	if(!initialized){
		__lwt_initialize();
	}

	if(!lwt || lwt->joinable == LWT_NOJOIN || lwt->parent_thd != curr_thd)
		return NULL;

	if(lwt->state == LWT_INFO_NTHD_RUNNABLE || lwt->state == LWT_INFO_NTHD_NEW ){

		lwt_block(curr_thd);
		
		//printf("lwt_join %d %d \n",wait_queue->size, run_queue->size);

		lwt_yield(LWT_NULL);
	}

	lwt->state = LWT_INFO_NTHD_FINISHED;

	thd_count--;

        return lwt->retVal;
}

void 
lwt_die(void *val)
{	
	//printf("lwt_die\n");

	if(curr_thd->joinable == LWT_JOIN){
		
		curr_thd->state = LWT_INFO_NTHD_ZOMBIES;

		curr_thd->retVal = val;	
		
		lwt_unblock(curr_thd->parent_thd);
	
		lwt_yield(LWT_NULL);
	}else{
		curr_thd->state = LWT_INFO_NTHD_FINISHED;

		thd_count--;
	}	
	
}

void 
lwt_block(lwt_t lwt)
{
	if(lwt->state == LWT_INFO_NTHD_BLOCKED)
		return;

	lwt->state = LWT_INFO_NTHD_BLOCKED;

	q_remove(run_queue,lwt->self_node);

	q_enqueue(wait_queue,lwt->self_node);


	//printf("lwt block %d %d \n",wait_queue->size, run_queue->size);

	//q_print(wait_queue);	

	//q_print(run_queue);	
}

void 
lwt_unblock(lwt_t lwt)
{
	if(lwt->state == LWT_INFO_NTHD_RUNNABLE)
		return;

	lwt->state = LWT_INFO_NTHD_RUNNABLE;

	q_remove(wait_queue,lwt->self_node);

	q_enqueue(run_queue,lwt->self_node);

	//printf("lwt unblock %d %d \n",wait_queue->size, run_queue->size);

	//q_print(wait_queue);	

	//q_print(run_queue);	
}

/* private lwt functions */

void
__lwt_initialize()
{
	//printf("lwt initialize: add main %x\n",tcb[temp]->state);
	
	if(initialized)
		return;

	initialized = 1;

	int temp = thd_count;

	run_queue = q_init();
	wait_queue =  q_init();
	
	tcb_index   = malloc(sizeof(lwt_tcb) * MAX_THREAD_SIZE);
	stack_index = malloc(DEFAULT_STACK_SIZE * MAX_THREAD_SIZE);	
	memset(tcb_index, 0, sizeof(tcb_index));
	memset(stack_index, 0, sizeof(stack_index));

	tcb[temp]         = tcb_index;
	tcb[temp]->tid    = temp;
	tcb[temp]->state = LWT_INFO_NTHD_RUNNABLE;
	tcb[temp]->self_node = q_make_node(tcb[temp]);
	tcb[temp]->group    = NULL;

	curr_thd = tcb[temp];

	thd_count++;
}

void 
__lwt_schedule(queue *run_queue, lwt_t next)
{	
	if(!run_queue || !run_queue->size)
		return;

	if(curr_thd->state == LWT_INFO_NTHD_RUNNABLE)
		q_enqueue(run_queue, curr_thd->self_node);

	if(!next)
		next = q_dequeue(run_queue)->data;
	else
		q_remove(run_queue, next->self_node);

	lwt_t curr = curr_thd;
	curr_thd = next;

	//printf("lwt schedule, curr %d next %d \n",curr->tid,next->tid);
	//printf("%d %d \n",wait_queue->size, run_queue->size);
	//q_print(run_queue);

	if(next->state == LWT_INFO_NTHD_NEW){
		next->state = LWT_INFO_NTHD_RUNNABLE;
	    	__lwt_dispatch_new(next, curr);
	}
	else{
		__lwt_dispatch(next, curr);
	}

}

void 
__lwt_dispatch(lwt_t next, lwt_t curr)
{
	__asm__ __volatile__(
	"pusha\n"
	"push   $1f\n"	
	"mov    0x8(%ebp),%eax\n"
  	"mov    0xc(%ebp),%edx\n"	  	
   	"mov    %ebp,0xc(%edx)\n"
   	"mov    %esp,0x10(%edx)\n"
	"mov    0x10(%eax),%esp\n"
   	"mov    0xc(%eax),%ebp\n"
	"ret\n"
	"1:popa\n" 			
                	   );
}


void 
__lwt_dispatch_new(lwt_t next, lwt_t curr) 
{
	__asm__ __volatile__(
	"pusha\n" 	
	"push   $2f\n" 
  	"mov    0xc(%ebp),%edx\n"
   	"mov    %esp,0x10(%edx)\n"
   	"mov    %ebp,0xc(%edx)\n"
	"mov    0x8(%ebp),%eax\n"	
	"mov    0xc(%eax),%edx\n"
   	"mov    0x18(%eax),%ecx\n"
   	"mov    0x1c(%eax),%eax\n"
   	"sub    $0x8,%edx\n"
   	"mov    %edx,%esp\n"
   	"mov    %ecx,(%esp)\n"
   	"mov    %eax,0x4(%esp)\n"
   	"call   __lwt_trampoline_test\n"			
  	"2:popa\n"
				);
}


void 
__lwt_start(lwt_fn_t fn, void * data)
{
	void * retVal = fn(data, curr_thd->chan);

	lwt_die(retVal);	
}

queue *
__run_queue()
{
	return run_queue;
}

queue *
__wait_queue()
{
	return wait_queue;
}
