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
	if(!c)
		return;

	lwt_t curr = lwt_current();

	c->ref_cnt--;


	if(!c->ref_cnt){
		DestroyDList(c->snd_list);
		free(c);

		//printf("lwt_chan_deref, free done\n");
	}

}

int 
lwt_snd(lwt_chan_t c, void *data)
{
	if(!c || c->rcv_thd == LWT_NULL)
		return -1;

	if(c->rcv_blocked){

		Node *n = MakeNode(lwt_current());

		AddNode(c->snd_list,n);

		//printf("thd id %d, lwt_snd blocked \n",lwt_current());

		lwt_yield(c->rcv_thd);
	}

	c->snd_data = data;
	
	//printf("thd id %d, lwt_snd done %x \n",lwt_current(), data);

	lwt_yield(c->rcv_thd);

	return 0;
}

void *
lwt_rcv(lwt_chan_t c)
{
	if(!c)
		return NULL;
	
	c->rcv_blocked = 0;

	//printf("thd id %d, lwt_rcv ready \n",lwt_current());

	if(c->snd_list->size){	
		Node *n = (lwt_t)GetNode(c->snd_list);
		lwt_yield(n->data);
	}else{
		lwt_yield(LWT_NULL);
	}

	c->rcv_blocked = 1;
	
	//printf("thd id %d, lwt_rcv done %x \n",lwt_current(), c->snd_data);

	void *data = c->snd_data;
	
	c->snd_data = NULL;

	return data;		
	
}

void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending)
{
	//printf("thd id %d, lwt_snd_chan \n", lwt_current());

	//Node *n = MakeNode(sending->rcv_thd);
	//AddNode(c->snd_list,n);


	//n = MakeNode(c->rcv_thd);
	//AddNode(sending->snd_list,n);

	//__print_chan(sending);
	//__print_chan(c);

	c->ref_cnt++;
	sending->ref_cnt++;

	lwt_snd(c,sending);

}

lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c)
{
	//printf("thd id %d, lwt_rcv_chan \n", lwt_current());

	lwt_chan_t cc = (lwt_chan_t)lwt_rcv(c);

	while(!cc){

		__queue_print(-1);

		lwt_yield(LWT_NULL);
		cc = (lwt_chan_t)lwt_rcv(c);
	}

	c->ref_cnt++;
	cc->ref_cnt++;

	return cc;
}

void
__print_chan(lwt_chan_t c)
{
	if(!c)
		return;
	
	printf("ref_cnt %d rcv_thd %d blokced %d\n",c->ref_cnt, c->rcv_thd, c->rcv_blocked);

	printf("snd_list \n");

	PrintDList(c->snd_list);


}

