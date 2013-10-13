#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>


#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

unsigned long long start, end;


/* Global Variables */

int counter = 0;

lwt_tcb *tcb[MAX_THREAD_SIZE];

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
	//printf("run queue remove: %d \n", thd->tid);


rdtscll(start);



	if(!thd)
		return -1;

	thd->prev->next = thd->next;

	if(thd->next)
		thd->next->prev = thd->prev;
	else
		queue_tail = NULL;

rdtscll(end);

printf("Overhead  of queue remove is %lld\n", (end-start));

	return 0;



}

int 
__queue_add(lwt_tcb *thd)
{
	//printf("run queue add: %d \n", thd->tid);
unsigned long long start, end;
rdtscll(start);
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

rdtscll(end);

printf("Overhead  of queue add is %lld\n", (end-start));
	return 0;
}

void 
__queue_print(void)
{	
	lwt_tcb *temp; 

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
	
	//add main tread
	if(!temp){

unsigned long long start1, end1;
rdtscll(start1);
		queue_head = malloc(sizeof(lwt_tcb));
rdtscll(end1);
		tcb[temp] = malloc(sizeof(lwt_tcb));

		tcb[temp]->tid    = temp;
		tcb[temp]->status = RUNNABLE;
		
		curr_thd = tcb[temp];
		//printf("lwt create: add main %x\n",tcb[temp]->status);
		temp++;
		counter++;

printf("in main is %lld\n", (end1-start1));
	}



	tcb[temp] = malloc(sizeof(lwt_tcb));



	tcb[temp]->stack  = malloc(DEFAULT_STACK_SIZE);




	tcb[temp]->tid    = temp;
	tcb[temp]->status = READY;
	
	tcb[temp]->bp     = tcb[temp]->stack + DEFAULT_STACK_SIZE - 0x4 ;
	tcb[temp]->fn     = fn;
	tcb[temp]->data   = data;
	tcb[temp]->next   = NULL;
	tcb[temp]->prev   = NULL;

unsigned long long start, end;
rdtscll(start);

	__queue_add(tcb[temp]);

rdtscll(end);
	counter++;



printf("Overhead  of lwt_create is %lld\n", (end-start));

	//fn(data);

	//printf("lwt create: stack %x \n", tcb[temp]->stack);

	return temp;
}

int 
lwt_yield(lwt_t lwt)
{
	//printf("lwt yield: \n");

	if(lwt == LWT_NULL){
		__lwt_schedule();
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
	//printf("lwt join: \n");

rdtscll(start);

	lwt_tcb *thd = tcb[lwt];


	while(thd->status != FINISHED){
		//printf("lwt join while: tid %x status %x \n",thd->tid,thd->status);
		lwt_yield(LWT_NULL);
	}
	void * retVal = thd->retVal;

	//printf("lwt join: stack %x\n", thd->stack);

	if(!thd->stack)
		free(thd->stack);
	//free(thd);

rdtscll(end);

printf("Overhead  of join is %lld\n", (end-start));
        return retVal;
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
	//printf("lwt schedule: \n");

	//__queue_print();

	if(!queue_head->next)
		return;


	//printf("next %d \n",queue_head->next->tid);

	lwt_tcb *prev_thd = curr_thd;
	curr_thd = queue_head->next;

	if(prev_thd->status != FINISHED)
		__queue_add(prev_thd);

	__queue_remove(curr_thd);

	__lwt_dispatch(curr_thd,prev_thd);
}

void 
__lwt_dispatch(lwt_tcb *next, lwt_tcb *curr)
{

	//printf("lwt_dispatch: next %x %x %x\n", next->tid, next->bp, next->sp);
	//printf("lwt_dispatch: curr %x %x %x\n", curr->tid, curr->bp, curr->sp);

	__asm__ __volatile__("push %0\n\t"
                             :
			     :"r"(&&return_here)
                             : 
                            );	

	__asm__ __volatile__("pushal\n\t"
			    );	
	__asm__ __volatile__("movl %%esp, %0\n\t"
                             :"=r"(curr->sp)
			     :
                             :
                            );
	__asm__ __volatile__("movl %%ebp, %0\n\t"
                             :"=r"(curr->bp)
			     :
                             :
                            );
	

	if(next->status == READY){
		next->status = RUNNABLE;
	    	__lwt_initial(next);		
	}
	else{	
		__asm__ __volatile__("mov %0,%%esp\n\t"
                             	     :
			             :"r"(next->sp)
                                     : 
                            	    );
		__asm__ __volatile__("mov %0,%%ebp\n\t"
                                     :
			     	     :"r"(next->bp)
                                     : 
                                    );
		__asm__ __volatile__("popal\n\t"
				    );	
		__asm__ __volatile__("ret\n\t"
				    );
	}
	

return_here:
	return;
}

void 
__lwt_initial(lwt_tcb *thd) 
{
	__asm__ __volatile__("subl $4,%esp\n\t"
                            );

	__asm__ __volatile__("movl %0,%%eax\n\t"
                             :
			     :"r"(thd->bp)
                             :"%eax"
                             );
	__asm__ __volatile__("movl %eax,0x8(%esp)\n\t"
                             );


	__asm__ __volatile__("mov %0,%%eax\n\t"
                             :
			     :"r"(thd->data)
                             :"%eax"
                             );
	__asm__ __volatile__("mov %eax,0x4(%esp)\n\t"
                             );

	__asm__ __volatile__("mov %0,%%eax\n\t"
                             :
			     :"r"(thd->fn)
                             :"%eax"
                             );
	__asm__ __volatile__("mov %eax,(%esp)\n\t"			
                             );

	__lwt_trampoline_test();
	//__lwt_trampoline();
}

void 
__lwt_start(lwt_fn_t fn, void * data)
{
       	//printf("__lwt_start: tid %d \n", curr_thd->tid);	

	void * retVal = fn(data);
	
	//printf("__lwt_start: tid %d retVal %d \n", curr_thd->tid, (int)retVal);

	lwt_die(retVal);

	lwt_yield(LWT_NULL);
}

int
lwt_info(lwt_info_t t)
{
	switch( t ) 
	{
		case LWT_INFO_NTHD_RUNNABLE:
       			return 0;
    		case LWT_INFO_NTHD_ZOMBIES:
       			return 0;
		case LWT_INFO_NTHD_BLOCKED:
       			return 0;
    		default :
        		return -1;
	}	
}
