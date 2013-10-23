#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lwt.h>

#define rdtscll(val) __asm__ __volatile__("rdtsc" : "=A" (val))


#define ITER 10000

/* 
 * My performance on an Intel Core i5-2520M CPU @ 2.50GHz:
 * Overhead for fork/join is 105
 * Overhead of yield is 26
 * Overhead of yield is 26
 */

int thd_cnt = 0;

void *
fn_bounce(void *d) 
{

	int i;
	unsigned long long start, end;

	thd_cnt++;	
	

	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);

	//printf("fn_bounce start %x\n",d);
	

	rdtscll(start);
	for (i = 0 ; i < ITER ; i++)
	{	 	
		lwt_yield(LWT_NULL);
	}
	rdtscll(end);

	//printf("fn_bounce end %x\n",d);

	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);

	//if (!d) 
	printf("Overhead of yield is %lld\n", (end-start)/ITER);


	return NULL;
}

void *
fn_null(void *d)
{ 
	thd_cnt++; 

	return d; 
}

void *
fn_identity(void *d)
{ 
	thd_cnt++; 
	//printf("fn_identity %x\n",d);
	return d; 
}

void *
fn_nested_joins(void *d)
{

	lwt_t chld;

	thd_cnt++;
	if (d) {
		
		lwt_yield(LWT_NULL);
		lwt_yield(LWT_NULL);
		//assert(lwt_info(LWT_INFO_NTHD_RUNNABLE) == 1);
		lwt_die(NULL);
	}
	
	//printf("fn_nested_joins \n");

	chld = lwt_create(fn_nested_joins, (void*)1);
	lwt_join(chld);
}

volatile int sched[2] = {0, 0};
volatile int curr = 0;

void *
fn_sequence(void *d)
{
	int i, other, val = (int)d;

	thd_cnt++;
	for (i = 0 ; i < ITER ; i++) {
		other = curr;
		curr  = (curr + 1) % 2;
		sched[curr] = val;
		assert(sched[other] != val);
		lwt_yield(LWT_NULL);
	}

	return NULL;
}

void *
fn_join(void *d)
{
	lwt_t t = (lwt_t)d;
	void *r;

	thd_cnt++;
	r = lwt_join(d);

	//printf("fn_join r: %x\n",r);
	//assert(r != (void*)0x37337);
	assert(r == (void*)0x37337);
}

#define IS_RESET()						\
        assert( lwt_info(LWT_INFO_NTHD_RUNNABLE) == 1 &&	\
		lwt_info(LWT_INFO_NTHD_ZOMBIES) == 0 &&		\
		lwt_info(LWT_INFO_NTHD_BLOCKED) == 0)

void * fn_my(void * data)
{
	//printf("fn my %x\n",data);

	lwt_yield(LWT_NULL);
}

int
main(void)
{	
	lwt_t chld1, chld2, chld3;
	int i;
	
	unsigned long long start, end,start2, end2;
/*
	
	chld1 = lwt_create(fn_bounce, 1);
	chld2 = lwt_create(fn_bounce, 2);
	lwt_join(chld1);
	lwt_join(chld2);
	lwt_yield(LWT_NULL);

	//IS_RESET();
	//thd_cnt + 2;
*/



	// Performance tests 
	
	
	rdtscll(start);
	for (i = 0 ; i < ITER ; i++) {		
		chld1 = lwt_create(fn_null,NULL);
		lwt_join(chld1);		
	}
	rdtscll(end);
	printf("Overhead for fork/join is %lld\n", (end-start)/ITER);

	IS_RESET();
	//thd_cnt + ITER;

	chld1 = lwt_create(fn_bounce, NULL);
	chld2 = lwt_create(fn_bounce, NULL);
	lwt_join(chld1);
	lwt_join(chld2);
	lwt_yield(LWT_NULL);

	IS_RESET();
	//thd_cnt + 2;

	// functional tests: scheduling 
	chld1 = lwt_create(fn_sequence, (void*)1);
	chld2 = lwt_create(fn_sequence, (void*)2);
	lwt_join(chld2);
	lwt_join(chld1);
	
	IS_RESET();
	//thd_cnt + 2;

	// functional tests: join 

	chld1 = lwt_create(fn_null, NULL);
	lwt_join(chld1);

	IS_RESET();
	//thd_cnt + 1;

	chld1 = lwt_create(fn_null, NULL);
	lwt_yield(LWT_NULL);
	lwt_join(chld1);

	IS_RESET();
	//thd_cnt + 1;
	

	chld1 = lwt_create(fn_nested_joins, NULL);
	lwt_join(chld1);

	IS_RESET();
	//thd_cnt + 2

	// functional tests: join only from parents 
	chld1 = lwt_create(fn_identity, (void*)0x37337);
	chld2 = lwt_create(fn_join, chld1);

	lwt_yield(LWT_NULL);
	lwt_yield(LWT_NULL);
	
	lwt_join(chld2);
	lwt_join(chld1);

	IS_RESET();
	//thd_cnt + 2
	
	// functional tests: passing data between threads 

	chld1 = lwt_create(fn_identity, (void*)0x37338);
	assert((void*)0x37338 == lwt_join(chld1));

	IS_RESET();
	//thd_cnt + 1


	// functional tests: directed yield 

	chld1 = lwt_create(fn_null, NULL);
	lwt_yield(chld1);
	assert(lwt_info(LWT_INFO_NTHD_ZOMBIES) == 1);
	lwt_join(chld1);

	IS_RESET();
	//thd_cnt + 1

	assert(thd_cnt == ITER+12);
/*
*/
	return 0;
}
