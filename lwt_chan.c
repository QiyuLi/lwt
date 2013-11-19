#include <stdio.h>
#include <stdlib.h>

#include <lwt.h>
#include <queue.h>
#include <ring_buffer.h>
#include <lwt_chan.h>

lwt_chan_t 
lwt_chan(int sz)
{
	lwt_chan_t c = malloc(sizeof(lwt_channel));

	if(!sz)
		sz = 1;

	c->snd_data_size = sz;
	c->snd_data      = rb_init(c->snd_data_size);

	c->snd_cnt       = 0;
	c->snd_list      = q_init();
	
	c->wait_queue    = q_init();

	c->rcv_thd       = lwt_current();
	c->rcv_blocked   = 1;

	c->group	 = NULL;

	return c;
}

void 
lwt_chan_deref(lwt_chan_t c)
{
	if(!c)
		return;
	
	//printf("lwt_chan_deref %d %d\n", c->snd_cnt, c->snd_list->size);


	if(c->rcv_thd == lwt_current() && !c->snd_cnt){
		q_free(c->snd_list);
		q_free(c->wait_queue);
		rb_free(c->snd_data);

		free(c);

		//printf("lwt_chan_deref, free done\n");
		return;
	}

	if(c->rcv_thd != lwt_current()){
		c->snd_cnt--;
	}

}

int 
lwt_snd(lwt_chan_t c, void *data)
{
	//printf("lwt snd \n");

	if(!c || c->rcv_thd == LWT_NULL)
		return -1;

	__lwt_chan_snd_event(c->group,c);

	while(1){
		if(!rb_full(c->snd_data)){

			rb_add(c->snd_data, data);
	
			lwt_unblock(c->rcv_thd);
		
			//printf("thd id %d, lwt_snd done %x \n",lwt_current()->tid, data);	
	
			return 0;
		}
		
		lwt_t curr = lwt_current();

		q_node *n = q_make_node(curr);

		q_enqueue(c->wait_queue,n);

		lwt_block(curr);	

		//printf("thd id %d, lwt_snd blocked\n",lwt_current()->tid);	

		lwt_yield(LWT_NULL);	
	}
	
}

void *
lwt_rcv(lwt_chan_t c)
{
	//printf("lwt rcv \n");

	if(!c)
		return NULL;

	__lwt_chan_rcv_event(c->group,c);

	while(1){

		if(!rb_empty(c->snd_data)){	

			void *data = rb_get(c->snd_data);
		
			//printf("thd id %d, lwt_rcv done %x \n",lwt_current()->tid, data);

			c->rcv_blocked = 0;

			return data;
		}

		if(!q_empty(c->wait_queue)){	
	
			q_node *n = q_dequeue(c->wait_queue);		

			lwt_unblock(n->data);	
		}

		//printf("thd id %d, lwt_rcv blocked\n",lwt_current()->tid);

		c->rcv_blocked = 0;

		lwt_block(c->rcv_thd);
		lwt_yield(LWT_NULL);
																																																																														
	}
}

void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending)
{
	sending->snd_cnt++;

	q_node *n  = q_make_node(c->rcv_thd);

	q_enqueue(sending->snd_list,n);

	lwt_snd(c,sending);
}

lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c)
{	
	lwt_chan_t cc = (lwt_chan_t)lwt_rcv(c);
							
	return cc;
}

void 
lwt_chan_mark_set(lwt_chan_t c, void *data)
{
	c->mark_data = data;
}

void *
lwt_chan_mark_get(lwt_chan_t c)
{
	return c->mark_data;
}

void
lwt_print_chan(lwt_chan_t c)
{
	if(!c)
		return;
	
	printf("ref_cnt %d rcv_thd %d blokced %d\n",c->snd_cnt, c->rcv_thd->tid, c->rcv_blocked);

	printf("snd_list \n");

	q_print(c->snd_list);
}



//lwt_chan_group function


lwt_cgrp_t 
lwt_cgrp(void)
{
	lwt_cgrp_t cg = malloc(sizeof(lwt_chan_grp));

	cg->snd_list = q_init();
	cg->rcv_list = q_init();
	cg->ready_list = q_init();
	cg->owner = lwt_current();

	return cg;
}

int 
lwt_cgrp_free(lwt_cgrp_t cg)
{
	if(!cg)
		return 0;

	if(!q_empty(cg->ready_list))
		return 1;
	
	q_free(cg->rcv_list);
	q_free(cg->snd_list);
	q_free(cg->ready_list);

	free(cg);

	return 0;
}

int 
lwt_cgrp_add(lwt_cgrp_t cg, lwt_chan_t c)
{	
	if(!cg || !c || c->group)
		return -1;

	//printf("lwt cgrp add\n");

	q_node *n = q_make_node(c);

	if(c->rcv_thd == lwt_current())
		q_enqueue(cg->rcv_list, n);
	else
		q_enqueue(cg->snd_list, n);

	c->group = cg;

	return 0;
}

int 
lwt_cgrp_rem(lwt_cgrp_t cg, lwt_chan_t c)
{
	if(!cg || !c)
		return -1;

	q_node *n = q_find_node(cg->ready_list,c);

	if(!n){

		if(c->rcv_thd == lwt_current())
			q_remove(cg->rcv_list, n);
		else
			q_remove(cg->snd_list, n);

		c->group = NULL;

		return 0;
	}else
		return 1;
}

void
lwt_print_cgrp(lwt_cgrp_t cg)
{
	if(!cg)
		return;
	
	printf("cg owner %d \n",cg->owner);

	q_node *n;

	printf("snd_chan %d\n", cg->snd_list->size);	
	for(n = cg->snd_list->head->next; n; n = n->next)
		lwt_print_chan(n->data);

	printf("rcv_chan %d\n", cg->rcv_list->size);	
	for(n = cg->rcv_list->head->next; n; n = n->next)
		lwt_print_chan(n->data);


}


lwt_chan_t 
lwt_cgrp_wait(lwt_cgrp_t cg)
{
	//printf("lwt_cgrp_wait, thd id %d \n",lwt_current()->tid);

	while(q_empty(cg->ready_list)){
		lwt_block(cg->owner);
		lwt_yield(LWT_NULL);
	}

	//printf("lwt_cgrp_wait unblocked, thd id %d \n",lwt_current()->tid);

	return q_dequeue(cg->ready_list)->data;
}



void 
__lwt_chan_snd_event(lwt_cgrp_t cg, lwt_chan_t c)
{
	if(!cg || !c)
		return;

	q_node *n = q_find_node(cg->rcv_list,c);

	if(n){
		//printf("lwt_chan_snd_event, thd id %d \n",lwt_current()->tid);

		n = q_make_node(c);
		q_enqueue(cg->ready_list, n);
		lwt_unblock(cg->owner);
	}
}

void 
__lwt_chan_rcv_event(lwt_cgrp_t cg, lwt_chan_t c)
{
	if(!cg || !c)
		return;

	q_node *n = q_find_node(cg->snd_list,c);

	if(n){
		//printf("lwt_chan_rcv_event, thd id %d \n",lwt_current()->tid);

		n = q_make_node(c);
		q_enqueue(cg->ready_list, n);
		lwt_unblock(cg->owner);
	}
}
