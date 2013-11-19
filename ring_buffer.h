#ifndef RING_BUFFER_H
#define RING_BUFFER_H

typedef struct ring_buffer
{
	int size;
	void **data;
	unsigned int get_index;
	unsigned int add_index;
} ring_buffer;


ring_buffer *
rb_init(int size);

int
rb_free(ring_buffer *rb);

int
rb_add(ring_buffer *rb, void *data);

int
rb_add_sync(ring_buffer *rb, void *data);

void *
rb_get(ring_buffer *rb);

void *
rb_get_sync(ring_buffer *rb);

int 
rb_full(ring_buffer *rb);

int 
rb_empty(ring_buffer *rb);

#endif
