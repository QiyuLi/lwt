#ifndef LWTCHAN_H
#define LWTCHAN_H

#include <lwt.h>
#include <DList.h>

typedef struct lwt_channel {
	int snd_data_size;
	void *snd_data;
	DList *snd_list;

	void *mark_data;
	int rcv_blocked;
	lwt_t rcv_thd;
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
__print_chan(lwt_chan_t c);

#endif
