#ifndef LWT_H
#define LWT_H

#include <d_linked_list.h>

#define MAX_THREAD_SIZE    512 
#define DEFAULT_STACK_SIZE 0x2000 // 16KB


/* Thread Status */

#define LWT_INFO_NTHD_NEW	0x1
#define LWT_INFO_NTHD_RUNNABLE	0x2
#define LWT_INFO_NTHD_ZOMBIES	0x3
#define LWT_INFO_NTHD_BLOCKED	0x4
#define LWT_INFO_NTHD_FINISHED	0x5

#define LWT_NULL NULL

#define LWT_NOJOIN 1
#define LWT_JOIN 0



typedef int lwt_tid;

typedef int lwt_info_t;

typedef int lwt_flags_t;

typedef void *(*lwt_fn_t)(void *);

typedef struct lwt_tcb {
        int tid;
        int status;
	void *stack;
	void *bp; 			//base pointer of stack
	void *sp;			//top pointer of satck
	void *group;	
	lwt_fn_t fn;
	void *data;
	void *retVal;			
	struct lwt_tcb *parent_thd; 
	lwt_flags_t joinable;
	void *self_node;
	
} lwt_tcb; 


typedef lwt_tcb *lwt_t;


typedef struct lwt_thd_group {

	dl_list *thd_list;
	dl_list *run_queue;
	dl_list *wait_queue;

	lwt_tcb *curr_thd;	

} lwt_thd_group;


typedef lwt_thd_group *lwt_tgrp_t;

/* lwt functions */
lwt_t 
lwt_create(lwt_fn_t fn, void *data, lwt_flags_t flag) ;

void *
lwt_join(lwt_t lwt);

void 
lwt_die(void *val);	

int 
lwt_yield(lwt_t makelwt);	

lwt_t 
lwt_current(void);	

int 
lwt_id(lwt_t lwt);		

int 
lwt_info(lwt_info_t t);

int 
lwt_block(lwt_t lwt);

int 
lwt_unblock(lwt_t lwt);


// lwt thd group functions

lwt_tgrp_t
lwt_tgrp();

int
lwt_tgrp_add(lwt_tgrp_t tg, lwt_t lwt);

int
lwt_tgrp_rem(lwt_tgrp_t tg, lwt_t lwt);

/* private lwt functions */

void 
__lwt_schedule(dl_list *run_queue, lwt_tcb *next);	

void 
__lwt_dispatch(lwt_tcb *next, lwt_tcb *curr) __attribute__ ((noinline));

void 
__lwt_dispatch_new(lwt_tcb *next, lwt_tcb *curr) __attribute__ ((noinline)); 

extern void 
__lwt_trampoline(); 

extern void 
__lwt_trampoline_test();

void 
__lwt_start(lwt_fn_t fn, void *data);	


dl_list *
__get_run_queue();

dl_list *
__get_wait_queue();

#endif
