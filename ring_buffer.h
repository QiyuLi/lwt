#ifndef RING_BUFFER_H
#define RING_BUFFER_H

typedef struct rb_array
{
	int size;
	int count;
	void **data;
	unsigned int get_index;
	unsigned int add_index;
} rb_array;


rb_array *
rb_init_array(int size);

int
rb_add_data(rb_array *array, void *data);

void *
rb_get_data(rb_array *array);

int 
rb_full(rb_array *array);

int 
rb_empty(rb_array *array);

#endif
