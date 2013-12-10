#ifndef LWT_H
#define LWT_H

#include <ring_buffer.h>

#define MAX_THREAD_SIZE    512 
#define DEFAULT_STACK_SIZE 0x2000 //

#define MAX_KP_SIZE 64


/* Thread Status */

#define LWT_INFO_NTHD_NEW	0x1
#define LWT_INFO_NTHD_RUNNABLE	0x2
#define LWT_INFO_NTHD_ZOMBIES	0x3
#define LWT_INFO_NTHD_BLOCKED	0x4
#define LWT_INFO_NTHD_FINISHED	0x5

#define LWT_NULL NULL

#define LWT_NOJOIN 1
#define LWT_JOIN 0

#define LWT_RUNNING 0x2

typedef int lwt_tid;

typedef int lwt_info_t;

typedef int lwt_flags_t;

typedef void *(*lwt_fn_t)(void *, void *);

typedef struct lwt_tcb *lwt_t;

typedef struct lwt_thd_group *lwt_tgrp_t;

typedef struct kernel_pool *kp_t;

typedef struct kp_request *kp_req_t;

struct lwt_channel;

struct lwt_tcb {
	void *bp; 			//base pointer of stack
	void *sp;			//top pointer of satck
	void *stack;

        int tid;
        int state;
	lwt_flags_t joinable;

	lwt_fn_t fn;
	void *data;
	void *chan;
	void *retVal;

	struct lwt_thd_group *group;			
        struct lwt_tcb *parent_thd; 
	
	void *self_node;
	
	struct lwt_tcb *rq_next;
	struct lwt_tcb *rq_prev;
	struct lwt_tcb *wq_next;
	struct lwt_tcb *wq_prev;

} lwt_tcb; 

struct lwt_thd_group {

	pthread_t pid;

	int thd_count;

	lwt_t tcb[MAX_THREAD_SIZE];

	void *tcb_index;
	void *stack_index;

	lwt_t rq_head;
	lwt_t wq_head;
	lwt_t curr_thd;

	ring_buffer *unblock_list;

} lwt_thd_group;

struct kthd_arg {
	lwt_fn_t fn;
	void* data;
	void* chan;
} kthd_arg;


struct kernel_pool {

	lwt_t master;

	void *master_chan;
	void *cg;
	int sz;
	int pthd_cnt;
	int act_cnt;

	ring_buffer *req_list;

	int exit;

} kernel_pool;

struct kp_request {
	lwt_fn_t fn;
	void *chan;
} kp_request;


/* lwt functions */
lwt_t 
lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flag, void *c) ;

void *
lwt_join(lwt_t lwt);

void 
lwt_die(void *val);	

int 
lwt_yield(lwt_t makelwt);	

lwt_t 
lwt_current();	

int 
lwt_id(lwt_t lwt);		

int 
lwt_info(lwt_info_t t);

void 
lwt_block(lwt_t lwt);

void 
lwt_unblock(lwt_t lwt);


// lwt kthd functions


int 
lwt_kthd_create(lwt_fn_t fn, void *data, void *c);

int
lwt_kthd_sndreq(lwt_tgrp_t tg, lwt_t lwt);

kp_t 
kp_create(int sz);

int
kp_work(kp_t pool, lwt_fn_t work, void *c);

int
kp_destroy(kp_t pool);

/* private lwt functions */

lwt_tgrp_t
__lwt_tgrp();

void 
__lwt_schedule(lwt_t next);	

void 
__lwt_dispatch(lwt_t next, lwt_t curr) __attribute__ ((noinline));

void 
__lwt_dispatch_new(lwt_t next, lwt_t curr) __attribute__ ((noinline)); 

extern void 
__lwt_trampoline(); 

extern void 
__lwt_trampoline_test();

void 
__lwt_start();

void 
__lwt_addrq(lwt_t thd);

void
__lwt_remrq(lwt_t thd);

void
__lwt_print_rq();

void
__lwt_print_wq();

#endif
