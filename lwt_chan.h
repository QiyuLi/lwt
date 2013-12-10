#ifndef LWT_CHAN_H
#define LWT_CHAN_H

#include <lwt.h>
#include <queue.h>
#include <ring_buffer.h>

#define LWT_CHAN_SND 0
#define LWT_CHAN_RCV 1
#define LWT_CHAN_ALL 2

typedef int lwt_chan_dir_t;

typedef struct lwt_channel *lwt_chan_t;

typedef struct lwt_chan_grp *lwt_cgrp_t;

typedef void (*chan_fn_t)(lwt_chan_t);

struct lwt_chan_grp {
	void *mark_data;
	lwt_t owner_thd;
	
	queue *sl;
	queue *rl;
	
	lwt_chan_t sl_head;
	lwt_chan_t sl_tail;
	
	lwt_chan_t rl_head;
	lwt_chan_t rl_tail;

	ring_buffer *req_list;

} lwt_chan_grp;

struct lwt_cgrp_req {
	chan_fn_t fn;
	lwt_chan_t c;
} lwt_cgrp_req;

struct lwt_channel {
	int snd_cnt;
	queue *snd_list;

	int snd_data_size;
	ring_buffer *snd_data;
	
	queue *wait_queue;	

	void *mark_data;
	lwt_t rcv_thd;

	lwt_cgrp_t snd_grp;
	lwt_cgrp_t rcv_grp;

	lwt_chan_t sl_next;
	lwt_chan_t sl_prev;
	lwt_chan_t rl_next;
	lwt_chan_t rl_prev;

} lwt_channel; 


lwt_chan_t 
lwt_chan(int sz);

void 
lwt_chan_deref(lwt_chan_t c);                         //not tested

void 
lwt_chan_ref(lwt_chan_t c);                         //not tested

int 
lwt_snd(lwt_chan_t c, void *data);


void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending);

void
lwt_snd_cdeleg(lwt_chan_t c, lwt_chan_t delegating);       //not tested

void *
lwt_rcv(lwt_chan_t c);


lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c);

lwt_chan_t 
lwt_rcv_cdeleg(lwt_chan_t c);                //not tested

void
lwt_print_chan(lwt_chan_t c);

void 
lwt_chan_mark_set(lwt_chan_t c, void *data);

void *
lwt_chan_mark_get(lwt_chan_t c);

//multi wait

lwt_cgrp_t 
lwt_cgrp(void);

int 
lwt_cgrp_free(lwt_cgrp_t cg);                 //not tested

int 
lwt_cgrp_add(lwt_cgrp_t cg, lwt_chan_t c, lwt_chan_dir_t direction);

int 
lwt_cgrp_rem(lwt_cgrp_t cg, lwt_chan_t c, lwt_chan_dir_t direction); //not tested

lwt_chan_t 
lwt_cgrp_wait(lwt_cgrp_t cg, lwt_chan_dir_t direction);

void
lwt_print_cgrp(lwt_cgrp_t cg);

//private function

void
__lwt_cgrp_sndreq(lwt_cgrp_t cg, chan_fn_t fn, lwt_chan_t c);

void 
__lwt_snd_event(lwt_chan_t c);

void 
__lwt_rcv_event(lwt_chan_t c);

void 
__lwt_chan_addsl(lwt_chan_t c);

void
__lwt_chan_remsl(lwt_chan_t c);

void 
__lwt_chan_addrl(lwt_chan_t c);

void
__lwt_chan_remrl(lwt_chan_t c);

#endif
