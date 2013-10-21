#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <lwt.h>

/* Global Variables */

int counter = 0;

lwt_tcb *tcb[MAX_THREAD_SIZE];

lwt_tcb *
__get_tcb(lwt_t lwt)
{
	return tcb[lwt];
}

lwt_tcb *curr_thd;

lwt_tcb *run_queue_head;
lwt_tcb *run_queue_tail;


/* run queue functions */

int 
__run_queue_remove(lwt_tcb* thd)
{
	//printf("run queue remove: %d \n", thd->tid);

	if(!thd)
		return -1;

	if(run_queue_head == run_queue_tail && thd->tid == run_queue_head->tid){
		run_queue_head = run_queue_tail = NULL;
		return 0;
	}
		
	
	if(thd->tid == run_queue_head->tid){
		run_queue_head = run_queue_head->next;	
		run_queue_head->prev = NULL;
		return 0;		
	}

	if(thd->tid == run_queue_tail->tid){
 		run_queue_tail = run_queue_tail->prev;
		run_queue_tail->next = NULL;
		return 0; 
	}

	thd->prev->next = thd->next;
	thd->next->prev = thd->prev;
	
	return 0;
}

int 
__run_queue_add(lwt_tcb* thd)
{

	if(!thd)
		return -1;
	
	if(run_queue_tail){
	    	run_queue_tail->next = thd;
		thd->prev = run_queue_tail;
		run_queue_tail = thd;
	}
	else{
	    	run_queue_tail = run_queue_head = thd;  
	}

	return 0;
}

void 
__run_queue_print(void)
{	
	if(curr_thd)
		printf("curr %d \n",curr_thd->tid);

	lwt_tcb *temp; 

	for(temp = run_queue_head; temp; temp = temp->next){   
	    printf("%d \n",temp->tid);
	}
	
	for(temp = run_queue_tail; temp; temp = temp->prev){   
	    printf("%d \n",temp->tid);
	}
}

/* lwt functions */


lwt_t 
lwt_create(lwt_fn_t fn, void *data)
{
	int temp = counter;
	
	//add main tread
	if(!temp){
		tcb[temp] = malloc(sizeof(lwt_tcb));
		tcb[temp]->tid    = temp;
		tcb[temp]->status = RUNNABLE;

		curr_thd = tcb[temp];
		//printf("lwt create: add main %x\n",tcb[temp]->status);
		temp++;
		counter++;	
	}

	tcb[temp] = malloc(sizeof(lwt_tcb));

	tcb[temp]->tid    = temp;
	tcb[temp]->status = READY;
	tcb[temp]->stack  = malloc(DEFAULT_STACK_SIZE);
	tcb[temp]->bp      = tcb[temp]->stack + DEFAULT_STACK_SIZE - 0x4 ;
	tcb[temp]->fn     = fn;
	tcb[temp]->data   = data;

	__run_queue_add(tcb[temp]);

	counter++;

	//fn(data);

	printf("lwt create: stack %x \n", tcb[temp]->stack);

	return temp;
}

int 
lwt_yield(lwt_t lwt)
{
	printf("lwt yield: \n");

	if(lwt == LWT_NULL){
		__lwt_schedule();
        	return 0;
	}

	lwt_tcb *prev_thd = curr_thd;
	curr_thd = tcb[lwt];

	__run_queue_remove(curr_thd);
	__run_queue_add(prev_thd);

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
	printf("lwt join: \n");

	lwt_tcb *thd = tcb[lwt];
	
	while(thd->status != FINISHED)
		lwt_yield(LWT_NULL);

	void * retVal = thd->retVal;

	printf("lwt join: stack %x\n", thd->stack);

	free(thd->stack);
	free(thd);


        return retVal;
}

void 
lwt_die(void *val)
{
	curr_thd->status = FINISHED;
	curr_thd->retVal = val;	
}

/* private lwt functions */

void 
__lwt_schedule(void)
{
	printf("lwt schedule: \n");

	if(!run_queue_head)
		return;

	lwt_tcb *prev_thd = curr_thd;
	curr_thd = run_queue_head;

	__run_queue_add(prev_thd);
	__run_queue_remove(curr_thd);

	__lwt_dispatch(curr_thd,prev_thd);
}

void 
__lwt_dispatch(lwt_tcb *next, lwt_tcb *curr)
{

	printf("lwt_dispatch: \n");
	
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
		__asm__ __volatile__("mov %0,%%ebp\n\t"
                                     :
			     	     :"r"(next->bp)
                                     : 
                                    );
		__asm__ __volatile__("mov %0,%%esp\n\t"
                             	     :
			             :"r"(next->sp)
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
       	printf("__lwt_start: tid %d \n", curr_thd->tid);	

	void * retVal = fn(data);
	
	printf("__lwt_start: tid %d retVal %d \n", curr_thd->tid, (int)retVal);

	lwt_die(retVal);

	lwt_yield(LWT_NULL);
}


