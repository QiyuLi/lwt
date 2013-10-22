#ifndef LWT_H
#define LWT_H

#define MAX_THREAD_SIZE 1024 
#define DEFAULT_STACK_SIZE 0x4000 // 16KB


/* Thread Status */
#define READY          0x00
#define ACTIVE	       0x01
#define RUNNABLE       0x02
#define FINISHED       0x10

//typedef int lwt_t;

#define LWT_NULL -1

typedef void *(*lwt_fn_t)(void *);

typedef struct lwt_tcb_struct {
        int tid;
        int status;
	void *stack;
	void *bp; 			//base pointer of stack
	void *sp;			//top pointer of satck
	lwt_fn_t fn;
	void * data;
	void * retVal;			//return value of fn
	struct lwt_tcb_struct *parent; 	//parent tcb
	struct lwt_tcb_struct *prev; 	//run queue prev pointer
	struct lwt_tcb_struct *next; 	//run queue next pointer
} lwt_tcb; 


//typedef lwt_tcb* lwt_t;

typedef int lwt_t;

typedef int lwt_tid;

#define LWT_INFO_NTHD_RUNNABLE	0x1
#define LWT_INFO_NTHD_ZOMBIES	0x2
#define LWT_INFO_NTHD_BLOCKED	0x4

typedef int lwt_info_t; //lwt info
#endif

/* lwt functions */
lwt_t 
lwt_create(lwt_fn_t fn, void *data) ; // done

void *
lwt_join(lwt_t lwt);	//TODO free memory

void 
lwt_die(void *val);	//done

int 
lwt_yield(lwt_t lwt);	//done

lwt_t 
lwt_current(void);	//done

int 
lwt_id(lwt_t lwt);		//done


/* private lwt functions */

void 
__lwt_schedule(void);	//done

void 
__lwt_dispatch(lwt_tcb *next, lwt_tcb *curr) ; //done

void 
__lwt_initial(lwt_tcb *thd) ; //done

extern void 
__lwt_trampoline(); 

extern void 
__lwt_trampoline_test(); //done

void 
__lwt_start() __attribute__ ((noinline));	//done

lwt_tcb *
__get_thread(lwt_t lwt);

void
__queue_print(void);

/* lwt info */
int lwt_info(lwt_info_t t);
