#ifndef LWT_H
#define LWT_H

#define MAX_THREAD_SIZE    512 
#define DEFAULT_STACK_SIZE 0x2000 // 16KB


/* Thread Status */

#define LWT_INFO_NTHD_NEW	0x1
#define LWT_INFO_NTHD_RUNNABLE	0x2
#define LWT_INFO_NTHD_ZOMBIES	0x3
#define LWT_INFO_NTHD_BLOCKED	0x4
#define LWT_INFO_NTHD_FINISHED	0x5

#define LWT_NULL -1

typedef void *(*lwt_fn_t)(void *);

typedef struct lwt_tcb_struct {
        int tid;
        int status;
	void *stack;
	void *bp; 			//base pointer of stack
	void *sp;			//top pointer of satck
	void *ip;                       //pointer of next instruction
	lwt_fn_t fn;
	void * data;
	void * retVal;			 //return value of fn
	struct lwt_tcb_struct *join_thd; //join tcb
	struct lwt_tcb_struct *prev; 	 //run queue prev pointer
	struct lwt_tcb_struct *next; 	 //run queue next pointer
} lwt_tcb; 


//typedef lwt_tcb* lwt_t;

typedef int lwt_t;

typedef int lwt_tid;

typedef int lwt_info_t;




/* lwt functions */
lwt_t 
lwt_create(lwt_fn_t fn, void *data) ;

void *
lwt_join(lwt_t lwt);

void 
lwt_die(void *val);	

int 
lwt_yield(lwt_t lwt);	

lwt_t 
lwt_current(void);	

int 
lwt_id(lwt_t lwt);		

int lwt_info(lwt_info_t t);

/* private lwt functions */

void 
__lwt_schedule(lwt_tcb *next);	

void 
__lwt_dispatch(lwt_tcb *next, lwt_tcb *curr) __attribute__ ((noinline));

void 
__lwt_initial(lwt_tcb *next, lwt_tcb *curr) __attribute__ ((noinline)); 

extern void 
__lwt_trampoline(); 

extern void 
__lwt_trampoline_test();

void 
__lwt_start(lwt_fn_t fn, void *data);	

lwt_tcb *
__get_thread(lwt_t lwt);

void
__queue_print(int i);

#endif
