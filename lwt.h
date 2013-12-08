#ifndef LWT_H
#define LWT_H

#include <ring_buffer.h>


#define MAX_THREAD_SIZE    512 
#define DEFAULT_STACK_SIZE 0x2000 //


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

	int thd_count;

	lwt_t tcb[MAX_THREAD_SIZE];

	void *tcb_index;
	void *stack_index;

	lwt_t rq_head;
	lwt_t wq_head;
	lwt_t curr_thd;

	ring_buffer *req_list;

} lwt_thd_group;




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


// lwt thd group functions

int 
lwt_kthd_create(lwt_fn_t fn, void *data, void *c);


lwt_tgrp_t
lwt_tgrp();

int
lwt_tgrp_add(lwt_tgrp_t tg, lwt_t lwt);

int
lwt_tgrp_rem(lwt_tgrp_t tg, lwt_t lwt);

lwt_tgrp_t
current_tgrp();


/* private lwt functions */

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
