#include <stdio.h>
#include <stdlib.h>
#include <lwtchan.h>
#include <lwt.h>
#include <DList.h>

lwt_chan_t 
lwt_chan(int sz)
{
	lwt_chan_t c = malloc(sizeof(lwt_channel));

	c->snd_data_size = sz;
	c->snd_data      = NULL;
	c->snd_list      = InitDList();
	c->rcv_thd       = lwt_current();
	c->rcv_blocked   = 1;

	return c;
}

void 
lwt_chan_deref(lwt_chan_t c)
{
	lwt_t curr = lwt_current();

	if(curr == c->rcv_thd)
		c->rcv_thd = LWT_NULL;
	else{
		Node *n = FindNode(c->snd_list,curr);

		RemoveNode(c->snd_list,n);
	}

	if(c->rcv_thd == LWT_NULL && !c->snd_list->size){
		DestroyDList(c->snd_list);
		free(c);
	}
		
}

int 
lwt_snd(lwt_chan_t c, void *data)
{
	if(!c || c->rcv_thd == LWT_NULL)
		return -1;

	while(c->rcv_blocked){
		printf("thd id %d, lwt_snd blocked \n",lwt_current());
		lwt_yield(c->rcv_thd);
	}

	c->snd_data = data;
	
	printf("thd id %d, lwt_snd done %x \n",lwt_current(), data);

	lwt_yield(c->rcv_thd);

	return 0;
}

void *
lwt_rcv(lwt_chan_t c)
{
	if(!c || !c->snd_list->size)
		return NULL;
		
	Node *n = (lwt_t)GetNode(c->snd_list);

	AddNode(c->snd_list,n);

	c->rcv_blocked = 0;

	printf("thd id %d, lwt_rcv ready \n",lwt_current());

	lwt_yield(n->data);

	c->rcv_blocked = 1;
	
	printf("thd id %d, lwt_rcv done %x \n",lwt_current(), c->snd_data);

	void *data = c->snd_data;
	
	c->snd_data = NULL;

	return data;		
	
}

void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending)
{
	printf("thd id %d, lwt_snd_chan \n", lwt_current());

	Node *n = MakeNode(sending->rcv_thd);
	AddNode(c->snd_list,n);


	n = MakeNode(c->rcv_thd);
	AddNode(sending->snd_list,n);

	//__print_chan(sending);
	//__print_chan(c);


	lwt_snd(c,sending);

}

lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c)
{
	printf("thd id %d, lwt_rcv_chan \n", lwt_current());

	lwt_chan_t cc = (lwt_chan_t)lwt_rcv(c);

	while(!cc){

		__queue_print(-1);

		lwt_yield(LWT_NULL);
		cc = (lwt_chan_t)lwt_rcv(c);
	}

	return cc;
}

void
__print_chan(lwt_chan_t c)
{
	if(!c)
		return;
	
	printf("rcv_thd %d blokced %d\n",c->rcv_thd, c->rcv_blocked);

	printf("snd_list \n");

	PrintDList(c->snd_list);


}

