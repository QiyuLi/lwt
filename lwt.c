#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>

#include <lwt.h>
#include <lwt_chan.h>
#include <ring_buffer.h>

//Performance test

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

volatile unsigned long long startx, endx;

//rdtscll(startx);
	
//rdtscll(endx);

//printf("Overhead of lwt_schedule is %lld \n", (endx-startx));


/* Global Variables */

__thread int initialized = 0;

__thread lwt_tgrp_t tg;


/* lwt functions */

int
lwt_info(lwt_info_t t)
{
	int i;
	int c[6] = {0,0,0,0,0,0,0};

	for(i = 0; i < MAX_THREAD_SIZE; i++)
		if(tg->tcb[i]) c[tg->tcb[i]->state]++;

	return c[t];
}

lwt_t 
lwt_current(void)
{
	if(tg)
        	return tg->curr_thd;
	else{
		__lwt_initialize();	
		return tg->curr_thd;
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
	lwt_tgrp_t tg = lwt_current()->group;

	if(tg->thd_count == MAX_THREAD_SIZE)
		return NULL;

	int temp;

	for(temp = 1; temp < MAX_THREAD_SIZE; temp++)
		if(!tg->tcb[temp] || tg->tcb[temp]->state == LWT_INFO_NTHD_FINISHED)
			break;
	
	//printf("lwt create temp = %d, data  = %x\n",temp, data);

	if(!tg->tcb[temp]){
		tg->tcb[temp] = tg->tcb_index + sizeof(lwt_tcb) * temp;
		tg->tcb[temp]->stack  = tg->stack_index + DEFAULT_STACK_SIZE * temp;
	}

	tg->tcb[temp]->tid      = temp;
	tg->tcb[temp]->state    = LWT_INFO_NTHD_NEW;	
	tg->tcb[temp]->bp       = tg->tcb[temp]->stack + DEFAULT_STACK_SIZE - 0x4 ;
	tg->tcb[temp]->fn       = fn;
	tg->tcb[temp]->data     = data;
	tg->tcb[temp]->joinable = flag;
	tg->tcb[temp]->parent_thd   = lwt_current();
	tg->tcb[temp]->group    = tg;

	if(c)	
		((lwt_chan_t)c)->rcv_thd = tg->tcb[temp];

	tg->tcb[temp]->chan     = c;

	tg->thd_count++;

	__lwt_addrq(tg->tcb[temp]);

	return tg->tcb[temp];
}

int 
lwt_yield(lwt_t lwt)
{
	//printf("lwt_yield %d %d \n",wait_queue->size, run_queue->size);

	if(!initialized){
		__lwt_initialize();
	}

	if(lwt == lwt_current())
		return 0;

	__lwt_schedule(lwt);

	return 0;
}

void *
lwt_join(lwt_t lwt)
{		
	lwt_t curr = lwt_current();

	if(!initialized){
		__lwt_initialize();
	}

	if(!lwt || lwt->joinable == LWT_NOJOIN || lwt->parent_thd != curr)
		return NULL;

	if(lwt->state == LWT_INFO_NTHD_RUNNABLE || lwt->state == LWT_INFO_NTHD_NEW ){

		lwt_block(curr);
		
		//printf("lwt_join %d %d \n",wait_queue->size, run_queue->size);

		lwt_yield(LWT_NULL);
	}

	lwt->state = LWT_INFO_NTHD_FINISHED;

	curr->group->thd_count--;

	

        return lwt->retVal;
}

void 
lwt_die(void *val)
{	
	//printf("lwt_die: %d\n", val);

	lwt_t curr = lwt_current();
	
	if(curr->joinable == LWT_JOIN){
		
		curr->state = LWT_INFO_NTHD_ZOMBIES;

		curr->retVal = val;	
		
		lwt_unblock(curr->parent_thd);
			
		lwt_yield(LWT_NULL);
	}else{
		curr->state = LWT_INFO_NTHD_FINISHED;

		curr->group->thd_count--;
	}	
	
}

void 
lwt_block(lwt_t lwt)
{
	if(lwt->state == LWT_INFO_NTHD_BLOCKED)
		return;

	lwt->state = LWT_INFO_NTHD_BLOCKED;

	__lwt_addwq(lwt);

	__lwt_remrq(lwt);


	//printf("lwt block %d \n",lwt->tid);
}

void 
lwt_unblock(lwt_t lwt)
{
	if(lwt->state == LWT_INFO_NTHD_RUNNABLE)
		return;

	lwt->state = LWT_INFO_NTHD_RUNNABLE;

	__lwt_addrq(lwt);

	__lwt_remwq(lwt);


	//printf("lwt unblock %d \n",lwt->tid);
}

lwt_tgrp_t
lwt_tgrp()
{
	lwt_tgrp_t tg = malloc(sizeof(lwt_thd_group));

	tg->thd_count = 0;

	tg->rq_head = tg->wq_head = NULL;

	tg->tcb_index   = malloc(sizeof(lwt_tcb) * MAX_THREAD_SIZE);
	tg->stack_index = malloc(DEFAULT_STACK_SIZE * MAX_THREAD_SIZE);	

	memset(tg->tcb_index, 0, sizeof(tg->tcb_index));
	memset(tg->stack_index, 0, sizeof(tg->stack_index));

	tg->req_list = rb_init(10);

	return tg;
}

struct multi_arg {
	lwt_fn_t fn;
	void* data;
	void* chan;
};

void *
__lwt_kthd_start(void *data)
{

	struct multi_arg *a = data;

	a->fn(a->data,a->chan);

	pthread_detach(pthread_self());
}

int
lwt_kthd_create(lwt_fn_t fn, void *data, void *c)
{
	pthread_t thd;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	struct multi_arg a;

	a.fn = fn;
	a.data = data;
	a.chan = c;

	int rc = pthread_create(&thd, &attr, __lwt_kthd_start, &a);

	

	//rc = pthread_join(thd, NULL);

	return rc;
}


/* private lwt functions */

void
__lwt_initialize()
{
	//printf("lwt initialize: add main %x\n",tcb[temp]->state);
	
	if(initialized)
		return;

	initialized = 1;

	tg = lwt_tgrp();
	
	int temp = tg->thd_count;
	tg->thd_count++;

	tg->tcb[temp]         = tg->tcb_index;
	tg->tcb[temp]->tid    = temp;
	tg->tcb[temp]->state  = LWT_INFO_NTHD_RUNNABLE;
	tg->tcb[temp]->group  = tg;

	__lwt_addrq(tg->tcb[temp]);

	tg->curr_thd = tg->tcb[temp];
}

void 
__lwt_schedule(lwt_t next)
{	
	lwt_t curr = lwt_current();	

	if(!next && curr->state == LWT_INFO_NTHD_RUNNABLE){
		next = curr->rq_next;
		curr->group->rq_head = next;
		curr->group->curr_thd = next;

		//printf("lwt schedule case 1, curr %d next %d \n",curr->tid,next->tid);
		//__lwt_print_rq();
		//__lwt_print_wq();
		
		if(next->state == LWT_INFO_NTHD_NEW){
			next->state = LWT_INFO_NTHD_RUNNABLE;
	    		__lwt_dispatch_new(next, curr);
		}
		else{
			__lwt_dispatch(next, curr);
		}	
		return;
	}

	if(curr->state == LWT_INFO_NTHD_ZOMBIES || curr->state == LWT_INFO_NTHD_FINISHED){
		__lwt_remrq(curr);
	}

	if(next){
		__lwt_remrq(next);
		__lwt_addrq(next);
	}else{
		next = curr->group->rq_head;	
	}
	
	curr->group->curr_thd = next;
	
	//printf("lwt schedule case 2, curr %d next %d \n",curr->tid,next->tid);
	//__lwt_print_rq();
	//__lwt_print_wq();

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
	"mov    %esp,0x4(%edx)\n"
   	"mov    %ebp,0x0(%edx)\n"
	"mov    0x4(%eax),%esp\n"
   	"mov    0x0(%eax),%ebp\n"
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
	"mov    0x8(%ebp),%eax\n"	
  	"mov    0xc(%ebp),%edx\n"
   	"mov    %esp,0x4(%edx)\n"
   	"mov    %ebp,0x0(%edx)\n"
	"mov    0x0(%eax),%esp\n"
   	"call   __lwt_trampoline_test\n"			
  	"2:popa\n"
				);
}

void 
__lwt_start()
{
	lwt_t curr = lwt_current();

	void * retVal = curr->fn(curr->data, curr->chan);

	lwt_die(retVal);	
}




void 
__lwt_addrq(lwt_t thd)
{
	lwt_t rq_head = thd->group->rq_head;

	if(!rq_head){
		thd->group->rq_head = thd;
		rq_head = thd->group->rq_head;

		rq_head->rq_next = rq_head->rq_prev = thd;
	}
	else{
		lwt_t rq_tail = rq_head->rq_prev;

		rq_tail->rq_next = thd;
		thd->rq_prev = rq_tail;

		thd->rq_next = rq_head;
		rq_head->rq_prev = thd;
	}

	//printf("addrq, head %d tail %d \n",rq_head->tid, thd->tid);
}

void
__lwt_remrq(lwt_t thd)
{	
	lwt_t rq_head = thd->group->rq_head;
	lwt_t rq_tail = thd->group->rq_head->rq_prev;

	if(thd == rq_head && thd == rq_tail){
		thd->group->rq_head = NULL;
		return;
	}
	if(thd == rq_head)
		thd->group->rq_head = thd->rq_next;
			
	thd->rq_prev->rq_next = thd->rq_next;
	thd->rq_next->rq_prev = thd->rq_prev;

	//printf("remrq, head %d tail %d \n",thd->group->rq_head->tid, thd->group->rq_head->rq_prev->tid);
}

void 
__lwt_addwq(lwt_t thd)
{
	lwt_t wq_head = thd->group->wq_head;

	if(!wq_head){
		thd->group->wq_head = thd;
		wq_head = thd->group->wq_head;

		wq_head->wq_next = wq_head->wq_prev = thd;
	}
	else{
		lwt_t wq_tail = wq_head->wq_prev;

		wq_tail->wq_next = thd;
		thd->wq_prev = wq_tail;

		thd->wq_next = wq_head;
		wq_head->wq_prev = thd;
	}
}

void
__lwt_remwq(lwt_t thd)
{
	lwt_t wq_head = thd->group->wq_head;
	lwt_t wq_tail = thd->group->wq_head->wq_prev;

	if(thd == wq_head && thd == wq_tail){
		thd->group->wq_head = NULL;
		return;
	}
	if(thd == wq_head)
		thd->group->wq_head = thd->wq_next;
			
	thd->wq_prev->wq_next = thd->wq_next;
	thd->wq_next->wq_prev = thd->wq_prev;
}

void
__lwt_print_rq()
{
	lwt_t curr = lwt_current();

	if(!curr){
		printf("run queue: empty  \n");
		return;
	}

	lwt_t thd;

	printf("run queue: %d ",curr->group->rq_head->tid);

	for(thd = curr->group->rq_head->rq_next; thd != curr; thd = thd->rq_next)
		printf("%d ",thd->tid);

	printf("\n");	
}

void
__lwt_print_wq()
{
	lwt_t wq_head = lwt_current()->group->wq_head;

	if(!wq_head){
		printf("wait queue: empty  \n");
		return;
	}

	lwt_t thd;
		
	printf("wait queue: %d ", wq_head->tid);

	for(thd = wq_head->wq_next; thd != wq_head; thd = thd->wq_next)
		printf("%d ",thd->tid);

	printf("\n");
}

