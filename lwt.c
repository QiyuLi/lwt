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


pthread_mutex_t k_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t k_cond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t kp_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t kp_cond = PTHREAD_COND_INITIALIZER;

//thread variables

__thread lwt_tgrp_t tg = NULL;


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
	if(!tg) __lwt_initialize();	
	return tg->curr_thd;
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

	if(c){	
		lwt_chan_t chan = (lwt_chan_t)c;
		chan->rcv_thd = tg->tcb[temp];
		lwt_chan_ref(chan);
		tg->tcb[temp]->chan     = c;
	}

	tg->thd_count++;
	__lwt_addrq(tg->tcb[temp]);

	return tg->tcb[temp];
}

int 
lwt_yield(lwt_t lwt)
{	
	lwt_t curr = lwt_current();
	
	if(lwt == curr)
		return 0;

	lwt_tgrp_t tg = curr->group;
	ring_buffer *rb = curr->group->unblock_list;	

	while(1){
		while(!rb_empty(rb))
			lwt_unblock(rb_get_sync(rb));
		
		if(tg->rq_head) break;

		//printf("pid %d lwt_yield: kthread blocked \n", curr->group->pid);
		
		pthread_cond_wait(&k_cond,&k_mutex);
		pthread_mutex_unlock(&k_mutex);
	}

	__lwt_schedule(lwt);

	return 0;
}

void *
lwt_join(lwt_t lwt)
{		
	lwt_t curr = lwt_current();

	if(!lwt || lwt->joinable == LWT_NOJOIN || lwt->parent_thd != curr)
		return NULL;

	if(lwt->state == LWT_INFO_NTHD_RUNNABLE || lwt->state == LWT_INFO_NTHD_NEW ){
		lwt_block(curr);
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
	__lwt_remrq(curr);

	if(curr->joinable == LWT_JOIN){		
		curr->state = LWT_INFO_NTHD_ZOMBIES;
		curr->retVal = val;			
		lwt_unblock(curr->parent_thd);			
	}else{
		curr->state = LWT_INFO_NTHD_FINISHED;
		curr->group->thd_count--;
	}	

	lwt_yield(LWT_NULL);	
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
	if(lwt->state == LWT_INFO_NTHD_RUNNABLE || lwt->state == LWT_INFO_NTHD_NEW)
		return;

	lwt->state = LWT_INFO_NTHD_RUNNABLE;
	__lwt_addrq(lwt);
	__lwt_remwq(lwt);

	//printf("lwt unblock %d \n",lwt->tid);
}

//kernel thread functions

void *
__lwt_kthd_start(struct kthd_arg *a)
{
	lwt_chan_t c = a->chan;

	if(c){
		c->rcv_thd = lwt_current();
		lwt_chan_ref(c);
	}

	//printf("pid %d _lwt_kthd_start %x %d %x\n",pthread_self(),a->fn,a->data,a->chan);

	a->fn(a->data,a->chan);

	free(a);

	pthread_detach(pthread_self());
}

int
lwt_kthd_create(lwt_fn_t fn, void *data, void *c)
{
	pthread_t thd;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	struct kthd_arg *a = malloc(sizeof(struct kthd_arg));
	a->fn = fn;
	a->data = data;
	a->chan = c;

	//printf("lwt_kthd_create %x %x %d %x \n",a, a->fn,a->data,a->chan);

	pthread_create(&thd, &attr, __lwt_kthd_start, a);

	return thd;
}

int
lwt_kthd_sndreq(lwt_tgrp_t tg, lwt_t lwt)
{
	ring_buffer *rb = tg->unblock_list;
	
	if(rb_full(rb))
		return -1;

	rb_add_sync(rb, lwt);
	
	//printf("pid %d lwt_kthd_sndreq %d\n",pthread_self(),lwt->tid);

	pthread_cond_broadcast(&k_cond);	

	//pthread_cond_signal(&k_cond);	

	return 0;
}

//kernel pool functions

void
__kp_worker(kp_t pool, lwt_chan_t c)
{
	while(1){

		__sync_fetch_and_add(&(pool->act_cnt),+1);
	
		//printf("pid %d __kp_worker, act_cnt %d\n",pthread_self(),pool->act_cnt);
	
		kp_req_t req = lwt_rcv(c);
	
		if((int)req == 0){
			break;	
		}

		__sync_fetch_and_add(&(pool->act_cnt),-1);
		req->fn(NULL,req->chan);
	}

	lwt_chan_deref(c);	
	
	printf("__kp_worker exit\n");

	pthread_detach(pthread_self());
}

void
__kp_master(kp_t pool, lwt_chan_t c)
{
	lwt_cgrp_t cg = lwt_cgrp();
	lwt_chan_t chan;

	int exit = 0;

	while(1){
		kp_req_t req = lwt_rcv(c);
	
		if((int)req == 0){
	
			while(pool->pthd_cnt)
				__kp_rem_worker(pool,cg);
			break;	
		}

		if(!pool->act_cnt && pool->pthd_cnt < pool->sz)
			__kp_add_worker(pool,cg);
		
		while( pool->act_cnt > (pool->sz + 1) / 2 )
			__kp_rem_worker(pool,cg);							
		
		chan = lwt_cgrp_wait(cg,LWT_CHAN_SND);
		lwt_snd(chan,req);	
	}	

	lwt_chan_deref(c);		
	free(pool);
	
	printf("__kp_master exit\n");

	pthread_detach(pthread_self());
}

void
__kp_add_worker(kp_t pool, lwt_cgrp_t cg)
{
	//printf("pid %d __kp_add_worker %d %d %d \n",pthread_self(), pool->act_cnt, pool->pthd_cnt, pool->sz);

	lwt_chan_t c = lwt_chan(0);		
	lwt_cgrp_add(cg, c, LWT_CHAN_SND);
	lwt_kthd_create(__kp_worker, pool, c);

	pool->pthd_cnt++;		
}

void
__kp_rem_worker(kp_t pool, lwt_cgrp_t cg)
{
	//printf("pid %d __kp_rem_worker, %d %d \n",pthread_self(), pool->act_cnt, pool->sz);	

	lwt_chan_t c = lwt_cgrp_wait(cg,LWT_CHAN_SND);			
	lwt_chan_deref(c);		
	lwt_cgrp_rem(cg, c, LWT_CHAN_SND);
			
	lwt_snd(c,0);

	pool->pthd_cnt--;	
	__sync_fetch_and_add(&(pool->act_cnt),-1);
}

kp_t 
kp_create(int sz)
{
	kp_t pool = malloc(sizeof(kernel_pool));

	pool->sz = sz;
	pool->act_cnt = 0;
	pool->exit = 0;
	pool->pthd_cnt = 0;
	pool->master_chan = lwt_chan(100);

	lwt_kthd_create(__kp_master, pool, pool->master_chan);

	return pool;
}

int
kp_work(kp_t pool, lwt_fn_t work, void *c)
{
	kp_req_t req = malloc(sizeof(kp_request));
	req->fn      = work;
	req->chan    = c;
	
	return lwt_snd(pool->master_chan,req);
}

int
kp_destroy(kp_t pool)
{
	return lwt_snd(pool->master_chan,0);
}

/* private lwt functions */

lwt_tgrp_t
__lwt_tgrp()
{
	lwt_tgrp_t tg = malloc(sizeof(lwt_thd_group));

	tg->pid = pthread_self();	
	tg->thd_count = 0;
	tg->rq_head = NULL;
	tg->wq_head = NULL;
	tg->unblock_list = rb_init(100);

	tg->tcb_index   = malloc(sizeof(lwt_tcb) * MAX_THREAD_SIZE);
	tg->stack_index = malloc(DEFAULT_STACK_SIZE * MAX_THREAD_SIZE);	

	memset(tg->tcb_index, 0, sizeof(tg->tcb_index));
	memset(tg->stack_index, 0, sizeof(tg->stack_index));

	return tg;
}

void
__lwt_initialize()
{
	//printf("lwt initialize: add main %x\n",tcb[temp]->state);
	
	if(tg) return;

	tg = __lwt_tgrp();
	
	int temp = tg->thd_count;

	tg->tcb[temp]         = tg->tcb_index;
	tg->tcb[temp]->tid    = temp;
	tg->tcb[temp]->state  = LWT_INFO_NTHD_RUNNABLE;
	tg->tcb[temp]->group  = tg;
	tg->curr_thd = tg->tcb[temp];

	tg->thd_count++;
	__lwt_addrq(tg->tcb[temp]);
}

void 
__lwt_schedule(lwt_t next)
{	
	lwt_t curr = lwt_current();	
	
	//printf("lwt schedule pid %d\n",curr->group->pid);

	if(!next && curr->state == LWT_INFO_NTHD_RUNNABLE){
		next = curr->rq_next;
		curr->group->rq_head = next;
		curr->group->curr_thd = next;

		if(next->state == LWT_INFO_NTHD_NEW){
			next->state = LWT_INFO_NTHD_RUNNABLE;
	    		__lwt_dispatch_new(next, curr);
		}
		else{
			__lwt_dispatch(next, curr);
		}	
		return;
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
	lwt_t rq_head = lwt_current()->group->rq_head;

	if(!rq_head){
		printf("run queue: empty  \n");
		return;
	}

	lwt_t thd;

	printf("run queue: %d ",rq_head->tid);

	for(thd = rq_head->rq_next; thd != rq_head; thd = thd->rq_next)
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

