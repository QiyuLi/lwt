#ifndef LWT_CHAN_H
#define LWT_CHAN_H

#include <lwt.h>
#include <queue.h>
#include <ring_buffer.h>


typedef struct lwt_chan_grp {
	queue *ready_list;
	queue *snd_list;
	queue *rcv_list;
	void *mark_data;
	lwt_t owner;
} lwt_chan_grp;

typedef struct lwt_chan_grp *lwt_cgrp_t;

typedef struct lwt_channel {
	int snd_cnt;
	queue *snd_list;

	int snd_data_size;
	ring_buffer *snd_data;
	
	queue *wait_queue;	

	void *mark_data;
	int rcv_blocked;
	lwt_t rcv_thd;

	lwt_cgrp_t group;
} lwt_channel; 

typedef struct lwt_channel *lwt_chan_t;

lwt_chan_t 
lwt_chan(int sz);

void 
lwt_chan_deref(lwt_chan_t c);

int 
lwt_snd(lwt_chan_t c, void *data);

void *
lwt_rcv(lwt_chan_t c);

void
lwt_snd_chan(lwt_chan_t c, lwt_chan_t sending);

lwt_chan_t 
lwt_rcv_chan(lwt_chan_t c);

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
lwt_cgrp_free(lwt_cgrp_t cg);

int 
lwt_cgrp_add(lwt_cgrp_t cg, lwt_chan_t c);

int 
lwt_cgrp_rem(lwt_cgrp_t cg, lwt_chan_t c);

lwt_chan_t 
lwt_cgrp_wait(lwt_cgrp_t cg);

void
lwt_print_cgrp(lwt_cgrp_t cg);

#endif
