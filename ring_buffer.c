#include <stdio.h>
#include <stdlib.h>

#include <cas.h>
#include <ring_buffer.h>

rb_array *
rb_init_array(int size)
{
	rb_array *array = malloc(sizeof(rb_array));

	array->data = malloc(sizeof(void *) * size);

	array->size = size;

	array->get_index = array->add_index = array->count = 0;

	return array;
}

int
rb_add_data(rb_array *array, void *data)
{
	if(array->count == array->size)
		return -1;

	array->data[array->add_index] = data;

	//printf("add index %d\n", array->add_index);

	array->add_index = ( ++ array->add_index) % array->size;
	
	array->count++;
}

void *
rb_get_data(rb_array *array)
{
	if(!array->count)
		return NULL;

	void *temp = array->data[array->get_index];

	//printf("get index %d\n", array->get_index);

	array->get_index = ( ++ array->get_index) % array->size;

	array->count--;

	return temp;
}

int 
rb_full(rb_array *array)
{
	return array->count == array->size;
}

int 
rb_empty(rb_array *array)
{
	return array->count == 0;
}
