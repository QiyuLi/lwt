#include <stdio.h>
#include <stdlib.h>

#include <ring_buffer.h>

ring_buffer *
rb_init(int size)
{
	ring_buffer *rb = malloc(sizeof(ring_buffer));

	rb->size = size + 1;

	rb->data = malloc(sizeof(void *) * rb->size);

	rb->get_index = 0;
	
	rb->add_index = 0;

	return rb;
}

int
rb_free(ring_buffer *rb)
{	
	if(rb){
		free(rb->data);
		free(rb);	
	}

	return 0;	
}

int
rb_add(ring_buffer *rb, void *data)
{
	if(!rb || rb_full(rb))
		return -1;

	rb->data[rb->add_index] = data;

	//printf("add index %d\n", rb->add_index);

	rb->add_index = (rb->add_index + 1) % rb->size;

	return 0;
}

int
rb_add_sync(ring_buffer *rb, void *data)
{
	int old_val, new_val;

	do{	
		if(!rb || rb_full(rb))
			return -1;

		old_val = rb->add_index;

		new_val = (old_val + 1) % rb->size;

	}while(!__sync_bool_compare_and_swap(&rb->add_index,old_val,new_val));

	//printf("add: add index %d, get index %d\n", rb->add_index, rb->get_index);

	rb->data[new_val] = data;

	return 0;
}

void *
rb_get(ring_buffer *rb)
{
	if(!rb || rb_empty(rb))
		return NULL;

	void *temp = rb->data[rb->get_index];

	//printf("get index %d\n", rb->get_index);

	rb->get_index = (rb->get_index + 1) % rb->size;

	return temp;
}

void *
rb_get_sync(ring_buffer *rb)
{
	int old_val, new_val;

	do{
		if(!rb || rb_empty(rb))
			return NULL;

		old_val = rb->get_index;

		new_val = (old_val + 1) % rb->size;

	}while(!__sync_bool_compare_and_swap(&rb->get_index,old_val,new_val));

	//printf("get: add index %d, get index %d\n", rb->add_index, rb->get_index);

	return rb->data[new_val];
}

int 
rb_full(ring_buffer *rb)
{
	if(!rb)
		return 1;

	return ((rb->add_index + 1) % rb->size ) == rb->get_index;
}

int 
rb_empty(ring_buffer *rb)
{
	if(!rb)
		return 1;

	return rb->add_index == rb->get_index;
}
