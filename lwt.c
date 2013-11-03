#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <lwt.h>


//Performance test

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))

volatile unsigned long long startx, endx;

//rdtscll(startx);
	
//rdtscll(endx);

//printf("Overhead of lwt_schedule is %lld \n", (endx-startx));




/* Global Variables */

int counter = 0;

lwt_tcb *tcb[MAX_THREAD_SIZE];
void *tcb_index;
void *stack_index;

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
	if(!thd)
		return -1;

	thd->prev->next = thd->next;

	if(thd->next)
		thd->next->prev = thd->prev;
	else
		queue_tail = thd->prev;

	thd->next = thd->prev = NULL;

	return 0;
}

int 
__queue_add(lwt_tcb *thd)
{
	if(!thd)
		return -1;
	
	if(queue_tail != queue_head){
	    	queue_tail->next = thd;
		thd->prev = queue_tail;	
	}
	else{
	    	queue_head->next = thd; 
		thd->prev = queue_head;
	}

	queue_tail = thd;

	return 0;
}

void 
__queue_print(int i)
{	
	lwt_tcb *temp; 

	
	lwt_tcb *prev = curr_thd;

	if(i>-1){
		curr_thd = tcb[i];

		__queue_remove(tcb[i]);
		__queue_add(prev);
	}

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

int
lwt_info(lwt_info_t t)
{
	int i;
	int c[6] = {0,0,0,0,0,0,0};

	for(i = 0; i < MAX_THREAD_SIZE; i++)
		if(tcb[i]) c[tcb[i]->status]++;

	return c[t];
}

lwt_t 
lwt_current(void)
{
	if(curr_thd)
        	return curr_thd->tid;
	else
		return 0;
}

int 
lwt_id(lwt_t lwt)
{
        return lwt;
}

lwt_t 
lwt_create(lwt_fn_t fn, void *data)
{

	if(counter == MAX_THREAD_SIZE)
		return -1;

	int temp = counter;

	if(!temp){
		tcb_index   = malloc(sizeof(lwt_tcb) * MAX_THREAD_SIZE);
		stack_index = malloc(DEFAULT_STACK_SIZE * MAX_THREAD_SIZE);
		queue_head  = queue_tail = malloc(sizeof(lwt_tcb));
		
		memset(tcb_index, 0, sizeof(tcb_index));
		memset(stack_index, 0, sizeof(stack_index));

		tcb[temp]         = tcb_index;
		tcb[temp]->tid    = temp;
		tcb[temp]->status = LWT_INFO_NTHD_RUNNABLE;
		
		curr_thd = tcb[temp];
		
		counter++;

		//printf("lwt create: add main %x\n",tcb[temp]->status);
	}

	for(temp = 1; temp < MAX_THREAD_SIZE; temp++)
		if(!tcb[temp] || tcb[temp]->status == LWT_INFO_NTHD_FINISHED)
			break;
	
	//printf("temp = %d, data  = %x\n",temp, data);

	if(!tcb[temp]){
		tcb[temp] = tcb_index + sizeof(lwt_tcb) * temp;
		tcb[temp]->stack  = stack_index + DEFAULT_STACK_SIZE * temp;
	}

	tcb[temp]->tid    = temp;
	tcb[temp]->status = LWT_INFO_NTHD_NEW;	
	tcb[temp]->bp     = tcb[temp]->stack + DEFAULT_STACK_SIZE - 0x4 ;
	tcb[temp]->fn     = fn;
	tcb[temp]->data   = data;
	tcb[temp]->next   = NULL;
	tcb[temp]->prev   = NULL;
	tcb[temp]->join_thd = NULL;
	counter++;

	__queue_add(tcb[temp]);

	return temp;
}

int 
lwt_yield(lwt_t lwt)
{
	//printf("lwt_yield \n");

	if(lwt == LWT_NULL)
		__lwt_schedule(NULL);
	else
		__lwt_schedule(tcb[lwt]);

	return 0;
}



void *
lwt_join(lwt_t lwt)
{		
	lwt_tcb *thd = tcb[lwt];

	lwt_tcb *curr = curr_thd;

	if(thd->status == LWT_INFO_NTHD_RUNNABLE || thd->status == LWT_INFO_NTHD_NEW ){

		curr->join_thd = thd;

		curr->status = LWT_INFO_NTHD_BLOCKED;

		__lwt_schedule(thd);
	}

	thd->status = LWT_INFO_NTHD_FINISHED;

	counter--;
	
        return thd->retVal;
}

void 
lwt_die(void *val)
{
	curr_thd->status = LWT_INFO_NTHD_ZOMBIES;

	curr_thd->retVal = val;	

	//printf("lwt_die\n");

	lwt_tcb *temp;

	for(temp =  queue_head->next; temp; temp = temp->next)
		if(temp->join_thd == curr_thd){
			temp->status = LWT_INFO_NTHD_RUNNABLE;
			temp->join_thd = NULL;
		}

	lwt_yield(LWT_NULL);
	
}

/* private lwt functions */


void 
__lwt_schedule(lwt_tcb *next)
{
	//printf("lwt schedule \n");

	if(!next){	
		for(next = queue_head->next; next; next = next->next)
			if(next->status == LWT_INFO_NTHD_NEW || 
			   next->status == LWT_INFO_NTHD_RUNNABLE  )
				break;

		if(!next)
			return;
	}

	lwt_tcb *curr = curr_thd;

	if(curr->status != LWT_INFO_NTHD_ZOMBIES)
		__queue_add(curr);

	__queue_remove(next);
	
	curr_thd = next;

	if(next->status == LWT_INFO_NTHD_NEW){
		next->status = LWT_INFO_NTHD_RUNNABLE;
	    	__lwt_initial(next, curr);
	}
	else{
		__lwt_dispatch(next, curr);
	}

}

void 
__lwt_dispatch(lwt_tcb *next, lwt_tcb *curr)
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
__lwt_initial(lwt_tcb *next, lwt_tcb *curr) 
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
	void * retVal = fn(data);

	lwt_die(retVal);	
}



lwt_tcb *
__get_thread(lwt_t lwt)
{
	return tcb[lwt];
}


