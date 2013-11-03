#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <lwt.h>
#include <lwtchan.h>
#include <DList.h>

void * bar(void *d)
{
	printf("thd id %d \n",lwt_current());

	lwt_yield(LWT_NULL);

	return NULL;
}


void testThread()
{
	lwt_t chd1 = lwt_create(bar,NULL);

	__queue_print(-1);

	lwt_yield(LWT_NULL);

	__queue_print(-1);

	lwt_t chd2 = lwt_create(bar,NULL);

	__queue_print(-1);

	lwt_yield(LWT_NULL);

	lwt_join(chd1);
	lwt_join(chd2);
}

void testDList()
{
	//printf("%x \n",n->data);

	DList *list = InitDList();
	PrintDList(list);

	AddNode(list,MakeNode(1));

	PrintDList(list);

	printf("%x \n",GetNode(list)->data);

	PrintDList(list);
}

void * foo1(void *d) {

	printf("chd id %d \n",lwt_current());

	int cnt = 0;
        lwt_chan_t c  = (lwt_chan_t)d;
	lwt_chan_t cc = lwt_chan(1);

	lwt_snd_chan(c, cc); // send parent our channel
	 
	printf("chd snd done \n");


	cnt = lwt_rcv(cc);

	printf("chd %d \n",cnt);

	lwt_snd(c, ++cnt);

	lwt_chan_deref(c);
	lwt_chan_deref(cc);
	 	
}

void 
testChanOne2One(void)
{
	int cnt = 0;
     	lwt_chan_t c  = lwt_chan(1);
     	lwt_t chd1    = lwt_create(foo1, c); // passing channel to thread	
     	lwt_chan_t cc1 = lwt_rcv_chan(c); // recv the child's channel
    

	printf("par done \n");

	//__print_chan(c);
	//__print_chan(cc);

	lwt_snd(cc1,++cnt);

     	cnt = lwt_rcv(c);
	
	printf("par %x \n",cnt);

	lwt_join(chd1);

	lwt_chan_deref(c);
	lwt_chan_deref(cc1);
}


void * foo2(void *d) {

	printf("chd id %d \n",lwt_current());

	int cnt = 0;
        lwt_chan_t c  = (lwt_chan_t)d;
	lwt_chan_t cc = lwt_chan(1);

	lwt_snd_chan(c, cc); // send parent our channel
	 
	printf("chd snd done \n");

	lwt_snd(c, ++cnt);
/*
	cnt = lwt_rcv(cc);
	
	while(!cnt)
		cnt = lwt_rcv(cc);

	printf("chd %d \n",cnt);
*/


	lwt_chan_deref(c);
	lwt_chan_deref(cc);
	 	
}

void 
testChanMany2One(void)
{
	int cnt        = 0;
     	lwt_chan_t c   = lwt_chan(1);

     	lwt_t chd1     = lwt_create(foo2, c); // passing channel to thread	
     	lwt_chan_t cc1 = lwt_rcv_chan(c); // recv the child's channel



	lwt_t chd2     = lwt_create(foo2, c); // passing channel to thread	
     	lwt_chan_t cc2 = lwt_rcv_chan(c); // recv the child's channel


	__print_chan(c);
	__print_chan(cc1);
	__print_chan(cc2);

	printf("par done \n");

	//lwt_snd(cc1,10);
	//lwt_snd(cc2,20);


     	cnt = lwt_rcv(c);

	printf("par %d \n",cnt);

	cnt = lwt_rcv(c);

	printf("par %d \n",cnt);

	lwt_join(chd1);
	lwt_join(chd2);

	lwt_chan_deref(c);
	lwt_chan_deref(cc1);
	lwt_chan_deref(cc2);
}

int main(void)
{

	//testDList();
	//testThread();
	//testChanOne2One();
	testChanMany2One();
	return 0;
}
