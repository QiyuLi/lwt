#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#include <lwt.h>
#include <lwt_chan.h>
#include <queue.h>
#include <ring_buffer.h>


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
	c->snd_grp = NULL;
	c->rcv_grp = NULL;
	c->sl_next = NULL;
	c->sl_prev = NULL;
	c->rl_next = NULL;
	c->rl_prev = NULL;

	return c;
}

void 
lwt_chan_deref(lwt_chan_t c)
{
	if(!c) return;
	
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
		__sync_fetch_and_add(&(c->snd_cnt),-1);
	}
}

void 
lwt_chan_ref(lwt_chan_t c)
{
	if(!c) return;
	__sync_fetch_and_add(&(c->snd_cnt),1);
}

int
lwt_snd(lwt_chan_t c, void *data)
{
	if(!c || c->rcv_thd == LWT_NULL)
		return -1;

	lwt_t curr = lwt_current();

	while(1){			
		if(!rb_full(c->snd_data)){

			rb_add_sync(c->snd_data, data);

			//printf("pid %d tid %d, lwt_snd done %d \n",curr->group->pid, curr->tid, data);	

			__lwt_snd_event(c);
	
			return 0;
		}

		q_node *n = q_make_node(curr);
		q_enqueue_sync(c->wait_queue,n);
		lwt_block(curr);

		//printf("pid %d tid %d, lwt_snd blocked %d \n",curr->group->pid, curr->tid, data);	

		lwt_yield(LWT_NULL);	
	}	
}

void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending)
{
	lwt_chan_ref(sending);

	q_node *n  = q_make_node(c->rcv_thd);
	q_enqueue(sending->snd_list,n);

	lwt_snd(c,sending);
}

void
lwt_snd_cdeleg(lwt_chan_t c, lwt_chan_t delegating)
{
	lwt_chan_ref(delegating);

	q_node *n  = q_make_node(lwt_current());
	q_enqueue(delegating->snd_list,n);

	lwt_snd(c,delegating);
}

void * 
lwt_rcv(lwt_chan_t c)
{	
	if(!c) return NULL;

	while(1){

		if(!rb_empty(c->snd_data)){	

			void *data = rb_get_sync(c->snd_data);

			//printf("pid %d, tid %d, lwt_rcv done %d \n",c->rcv_thd->group->pid, c->rcv_thd->tid, data);

			__lwt_rcv_event(c);
			
			return data;
		}
		
		//printf("pid %d, tid %d, lwt_rcv blocked\n",c->rcv_thd->group->pid,c->rcv_thd->tid);

		lwt_block(c->rcv_thd);
		lwt_yield(LWT_NULL);
																																																																														
	}
}

lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c)
{	
	lwt_chan_t cc = lwt_rcv(c);						
	return cc;
}

lwt_chan_t 
lwt_rcv_cdeleg(lwt_chan_t c)
{
	lwt_chan_t cc = lwt_rcv(c);
	cc->rcv_thd = lwt_current();	
			
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
	if(!c) return;
	
	printf("ref_cnt %d rcv_thd %d buffer size %d\n",c->snd_cnt, c->rcv_thd->tid, c->snd_data->size-1);
}

//lwt_chan_group function

lwt_cgrp_t 
lwt_cgrp(void)
{
	lwt_cgrp_t cg = malloc(sizeof(lwt_chan_grp));

	cg->owner_thd = lwt_current();
	cg->sl      = q_init();
	cg->rl      = q_init();

	cg->sl_head = malloc(sizeof(lwt_channel));
	cg->sl_head->sl_next = NULL;
	cg->sl_tail = cg->sl_head;
	
	cg->rl_head = malloc(sizeof(lwt_channel));
	cg->rl_head->rl_next = NULL;
	cg->rl_tail = cg->rl_head;
	
	cg->req_list      = rb_init(100);

	return cg;
}

int 
lwt_cgrp_free(lwt_cgrp_t cg)
{
	if(!cg)
		return -1;

	__lwt_cgrp_hndreq(cg);

	if(cg->rl_head->rl_next)
		return -1;

	q_node *n;
	lwt_chan_t c;

	for(n = cg->rl->head->next; n; n = n->next){
		c = n->data;
		c->rcv_grp = NULL;
	}
	for(n = cg->sl->head->next; n; n = n->next){
		c = n->data;
		c->snd_grp = NULL;
	}

	free(cg->sl_head);
	free(cg->rl_head);
	q_free(cg->sl);
	q_free(cg->rl);

	free(cg);

	return 0;
}

int 
lwt_cgrp_add(lwt_cgrp_t cg, lwt_chan_t c, lwt_chan_dir_t direction)
{	
	if(!cg || !c)
		return -1;

	//printf("lwt cgrp add\n");

	if(direction == LWT_CHAN_SND){
		if(c->snd_grp)
			return -1;

		c->snd_grp = cg;

		q_enqueue(cg->sl,q_make_node(c));

		if(!rb_full(c->snd_data))
			__lwt_chan_addsl(c);
	}else{
		if(c->rcv_grp)
			return -1;

		c->rcv_grp = cg;

		q_enqueue(cg->rl,q_make_node(c));
		
		if(!rb_empty(c->snd_data))
			__lwt_chan_addrl(c);
	}

	return 0;
}

int 
lwt_cgrp_rem(lwt_cgrp_t cg, lwt_chan_t c, lwt_chan_dir_t direction)
{
	if(!cg || !c)
		return -1;

	if(direction == LWT_CHAN_SND){
		if(cg != c->snd_grp)
			return -1;

		__lwt_chan_remsl(c);		
		c->snd_grp = NULL;	
	}else{
		if(cg != c->rcv_grp)
			return -1;

		__lwt_chan_remrl(c);
		c->rcv_grp = NULL;	
	}

	return 0;
}


void
lwt_print_cgrp(lwt_cgrp_t cg)
{
	if(!cg) return;
	
	printf("cg owner %d\n",cg->owner_thd->tid);

	lwt_chan_t c;

	printf("snd_chan: ");	
	for(c = cg->sl_head->sl_next; c; c = c->sl_next)
		printf(" %x ",c);

	printf("\nrcv_chan: ");	
	for(c = cg->rl_tail; c != cg->rl_head; c = c->rl_prev)
		printf(" %x ",c);

	printf("\n");	
}

lwt_chan_t 
lwt_cgrp_wait(lwt_cgrp_t cg, lwt_chan_dir_t direction)
{
	//printf("pid %d tid %d lwt_cgrp_wait %d\n",pthread_self(), lwt_current()->tid,rb_empty(cg->req_list));

	lwt_chan_t c = NULL;

	while(1){
		__lwt_cgrp_hndreq(cg);

		if(direction == LWT_CHAN_SND)
			c = cg->sl_head->sl_next;
		else
			c = cg->rl_head->rl_next;
	
		if(c){
			//printf("lwt_cgrp_wait unblocked, thd id %d %x\n",lwt_current()->tid, c);
			return c;
		}

		lwt_block(cg->owner_thd);
		lwt_yield(LWT_NULL);
	}
}

//private function

void
__lwt_cgrp_hndreq(lwt_cgrp_t cg)
{
	while(!rb_empty(cg->req_list)){
		struct lwt_cgrp_req *req = rb_get_sync(cg->req_list);

		//printf("__lwt_cgrp_hndreq %x %x \n", req->fn, req->c);

		req->fn(req->c);
		free(req);
	}
}

void
__lwt_cgrp_sndreq(lwt_cgrp_t cg, chan_fn_t fn, lwt_chan_t c)
{
	struct lwt_cgrp_req *req = malloc(sizeof(lwt_cgrp_req));
	
	req->fn = fn;
	req->c  = c;

	rb_add_sync(cg->req_list,req);

	//printf("pid %d tid %d __lwt_cgrp_sndreq, %x\n",pthread_self(),lwt_current()->tid, c);
}

void 
__lwt_snd_event(lwt_chan_t c)
{
	//printf("pid %d tid %d lwt_snd_event_kthd, %x\n",pthread_self(),lwt_current()->tid, c);

	if(c->rcv_grp){
		__lwt_cgrp_sndreq(c->rcv_grp,__lwt_chan_addrl,c);

		//lwt_kthd_sndreq(c->rcv_grp->owner_thd->group,c->rcv_grp->owner_thd);
	}

	if(c->snd_grp && rb_full(c->snd_data)){
		
		__lwt_cgrp_sndreq(c->snd_grp,__lwt_chan_remsl,c);
	}
	
	lwt_kthd_sndreq(c->rcv_thd->group,c->rcv_thd);
}

void 
__lwt_rcv_event(lwt_chan_t c)
{
	//printf("pid %d tid %d lwt_rcv_event_kthd, %x\n",pthread_self(),lwt_current()->tid, c);

	if(c->snd_grp){
		__lwt_cgrp_sndreq(c->snd_grp,__lwt_chan_addsl,c);
	
		lwt_kthd_sndreq(c->snd_grp->owner_thd->group, c->snd_grp->owner_thd);
	}

	if(c->rcv_grp && rb_empty(c->snd_data)){
		//printf("rem req %x \n",c);
		__lwt_cgrp_sndreq(c->rcv_grp,__lwt_chan_remrl,c);
	}

	if(!q_empty(c->wait_queue)){	
		q_node *n = q_dequeue_sync(c->wait_queue);		
		lwt_t thd = (lwt_t)n->data;		
		lwt_kthd_sndreq(thd->group, thd);
	}
}

void 
__lwt_chan_addsl(lwt_chan_t c)
{
	if(!c || c->sl_prev)
		return;

	c->snd_grp->sl_tail->sl_next = c;
	c->sl_prev = c->snd_grp->sl_tail;
	c->snd_grp->sl_tail = c;
	c->snd_grp->sl_tail->sl_next = NULL;
}

void
__lwt_chan_remsl(lwt_chan_t c)
{
	if(!c || !c->sl_prev)
		return;

	c->sl_prev->sl_next = c->sl_next;

	if(c == c->snd_grp->sl_tail)
		//c->snd_grp->sl_tail = c->snd_grp->sl_head;
		c->snd_grp->sl_tail = c->sl_prev;
	else
		c->sl_next->sl_prev = c->sl_prev;	

	c->sl_next = c->sl_prev = NULL;	
}

void 
__lwt_chan_addrl(lwt_chan_t c)
{
	if(!c || c->rl_prev)
		return;

	c->rcv_grp->rl_tail->rl_next = c;
	c->rl_prev = c->rcv_grp->rl_tail;
	c->rcv_grp->rl_tail = c;
	c->rcv_grp->rl_tail->rl_next = NULL;

	//printf("addrl %x\n",c);
}

void
__lwt_chan_remrl(lwt_chan_t c)
{
	//printf("__lwt_chan_remrl %x \n",c);

	if(!c || !c->rl_prev)
		return;

	c->rl_prev->rl_next = c->rl_next;

	if(c == c->rcv_grp->rl_tail)
		//c->rcv_grp->rl_tail = c->rcv_grp->rl_head;
		c->rcv_grp->rl_tail = c->rl_prev;
	else
		c->rl_next->rl_prev = c->rl_prev;	

	c->rl_next = c->rl_prev = NULL;	

}
