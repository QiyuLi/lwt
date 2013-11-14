#include <stdio.h>
#include <stdlib.h>

#include <cas.h>
#include <ring_buffer.h>

rb_array *
rb_init_array(int size)
{
	rb_array *array = malloc(sizeof(rb_array));

	array->size = size + 1;

	array->data = malloc(sizeof(void *) * array->size);

	array->get_index = 0;
	
	array->add_index = 0;

	return array;
}

int
rb_add_data(rb_array *array, void *data)
{
	if(rb_full(array))
		return -1;

	array->data[array->add_index] = data;

	//printf("add index %d\n", array->add_index);

	array->add_index = ( ++ array->add_index) % array->size;

	return 0;
}

void *
rb_get_data(rb_array *array)
{
	if(rb_empty(array))
		return NULL;

	void *temp = array->data[array->get_index];

	//printf("get index %d\n", array->get_index);

	array->get_index = ( ++ array->get_index) % array->size;

	return temp;
}

int
rb_add_data_sync(rb_array *array, void *data)
{
	int old_val, new_val;

	do{	
		if(rb_full(array))
			return -1;

		old_val = array->add_index;

		new_val = (old_val + 1) % array->size;

	}while(!__sync_bool_compare_and_swap(&array->add_index,old_val,new_val));

	//printf("add: add index %d, get index %d\n", array->add_index, array->get_index);

	array->data[new_val] = data;

	return 0;
}

void *
rb_get_data_sync(rb_array *array)
{
	int old_val, new_val;

	do{
		if(rb_empty(array))
			return NULL;

		old_val = array->get_index;

		new_val = (old_val + 1) % array->size;

	}while(!__sync_bool_compare_and_swap(&array->get_index,old_val,new_val));

	//printf("get: add index %d, get index %d\n", array->add_index, array->get_index);

	return array->data[new_val];
}


int 
rb_full(rb_array *array)
{
	return ((array->add_index + 1) % array->size ) == array->get_index;
}

int 
rb_empty(rb_array *array)
{
	return array->add_index == array->get_index;
}
