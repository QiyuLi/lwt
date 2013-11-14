#include <stdio.h>
#include <stdlib.h>

#include <lwt.h>
#include <d_linked_list.h>
#include <ring_buffer.h>
#include <lwt_chan.h>

lwt_chan_t 
lwt_chan(int sz)
{
	lwt_chan_t c = malloc(sizeof(lwt_channel));

	c->snd_data_size = sz;
	c->snd_data      = rb_init_array(sz);
	c->snd_list      = dl_init_list();
	c->rcv_thd       = lwt_current();
	c->group	 = NULL;
	//c->rcv_blocked   = 1;

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
		dl_destroy_list(c->snd_list);
		free(c);
		//printf("lwt_chan_deref, free done\n");
	}

}

int 
lwt_snd(lwt_chan_t c, void *data)
{
	if(!c || c->rcv_thd == LWT_NULL)
		return -1;

	while(1){
		if(!rb_full(c->snd_data)){

			rb_add_data(c->snd_data, data);
	
			lwt_unblock(c->rcv_thd);

			__lwt_chan_snd_event(c->group,c);

			//printf("thd id %d, lwt_snd done %x \n",lwt_current()->tid, data);
			return 0;
		}
		
		lwt_t curr = lwt_current();

		dl_node *n = dl_make_node(curr);

		dl_add_node(c->snd_list,n);

		lwt_block(curr);	

		//printf("thd id %d, lwt_snd blocked, yield to thd id %d \n",lwt_current()->tid, c->rcv_thd->tid);	

		lwt_yield(LWT_NULL);	
	}
	
}

void *
lwt_rcv(lwt_chan_t c)
{
	//printf("lwt rcv \n");

	if(!c)
		return NULL;

	while(1){

		if(!rb_empty(c->snd_data)){	
			void *data = rb_get_data(c->snd_data);
		
			//printf("thd id %d, lwt_rcv done %x \n",lwt_current()->tid, data);

			__lwt_chan_rcv_event(c->group,c);

			return data;
		}

		if(!dl_empty(c->snd_list)){	
	
			dl_node *n = dl_get_node(c->snd_list);		

			lwt_unblock(n->data);	
			//printf("thd id %d, lwt_rcv unblock thd id %d \n",lwt_current()->tid, ((lwt_t)n->data)->tid);
		}

		//printf("thd id %d, lwt_rcv blocked\n",lwt_current()->tid);
		lwt_block(c->rcv_thd);
		lwt_yield(LWT_NULL);
																																																																														
	}
}

void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending)
{
	c->ref_cnt++;
	sending->ref_cnt++;

	lwt_snd(c,sending);
}

lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c)
{	
	lwt_chan_t cc = (lwt_chan_t)lwt_rcv(c);
							
	c->ref_cnt++;
	cc->ref_cnt++;

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
__print_chan(lwt_chan_t c)
{
	if(!c)
		return;
	
	printf("ref_cnt %d rcv_thd %d blokced %d\n",c->ref_cnt, c->rcv_thd->tid, c->rcv_blocked);

	printf("snd_list \n");

	dl_print_list(c->snd_list);
}



//lwt_chan_group function


lwt_cgrp_t 
lwt_cgrp(void)
{
	lwt_cgrp_t cg = malloc(sizeof(lwt_chan_grp));

	cg->snd_list = dl_init_list();
	cg->rcv_list = dl_init_list();
	cg->ready_list = dl_init_list();
	cg->owner = lwt_current();

	return cg;
}

int 
lwt_cgrp_free(lwt_cgrp_t cg)
{
	if(!dl_empty(cg->ready_list))
		return 1;

	dl_destroy_list(cg->rcv_list);
	dl_destroy_list(cg->snd_list);
	dl_destroy_list(cg->ready_list);

	free(cg);

	return 0;
}

int 
lwt_cgrp_add(lwt_cgrp_t cg, lwt_chan_t c)
{	
	if(!cg || !c || !c->group)
		return -1;

	dl_node *n = dl_make_node(c);

	if(c->rcv_thd == lwt_current())
		dl_add_node(cg->rcv_list, n);
	else
		dl_add_node(cg->snd_list, n);

	c->group = cg;

	return 0;
}

int 
lwt_cgrp_rem(lwt_cgrp_t cg, lwt_chan_t c)
{
	if(!cg || !c)
		return -1;

	dl_node *n = dl_find_node(cg->ready_list,c);

	if(!n){

		if(c->rcv_thd == lwt_current())
			dl_remove_node(cg->rcv_list, n);
		else
			dl_remove_node(cg->snd_list, n);

		c->group = NULL;

		return 0;
	}else
		return 1;
}

lwt_chan_t 
lwt_cgrp_wait(lwt_cgrp_t cg)
{
	while(dl_empty(cg->ready_list)){
		lwt_block(cg->owner);
		lwt_yield(LWT_NULL);
	}

	return dl_get_node(cg->ready_list)->data;
}



void 
__lwt_chan_snd_event(lwt_cgrp_t cg, lwt_chan_t c)
{
	if(!cg || !c)
		return;

	dl_node *n = dl_find_node(cg->rcv_list,c);

	if(n){
		n = dl_make_node(c);
		dl_add_node(cg->ready_list, n);
		lwt_unblock(cg->owner);
	}
}

void 
__lwt_chan_rcv_event(lwt_cgrp_t cg, lwt_chan_t c)
{
	if(!cg || !c)
		return;

	dl_node *n = dl_find_node(cg->snd_list,c);

	if(n){
		n = dl_make_node(c);
		dl_add_node(cg->ready_list, n);
		lwt_unblock(cg->owner);
	}
}
