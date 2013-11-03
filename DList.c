#include <stdio.h>
#include <stdlib.h>

#include <DList.h>


dl_node *
dl_make_node(void *data)
{
	dl_node *n = malloc(sizeof(dl_node));
	
	n->data = data;
	n->next = n->prev = NULL;

	return n;
}

dl_list *
dl_init_list()
{
	dl_list *list = malloc(sizeof(dl_list));	
	
	list->head = list->tail = dl_make_node(NULL);

	list->size = 0;

	return list;
}

void 
dl_destroy_list(dl_list *list)
{
	dl_clear_list(list);

	free(list->head);

	free(list);
}

void
dl_clear_list(dl_list *list)
{
	dl_node *n = list->tail;
	dl_node *temp;

	while(list->size > 0){	
		temp = n;
		n = n->prev;
		free(temp);
		list->size--;
	}
	
	list->tail = list->head;
}


int 
dl_remove_node(dl_list *list, dl_node *node)
{
	if(!list || !node || !node->prev)
		return -1;
	
	node->prev->next = node->next;

	if(node->next)
		node->next->prev = node->prev;
	else
		list->tail = node->prev;

	node->next = node->prev = NULL;

	list->size--;

	return 0;
}

int 
dl_add_node(dl_list *list, dl_node *node)
{
	if(!list || !node)
		return -1;
	
	if(list->tail != list->head){
	    	list->tail->next = node;
		node->prev = list->tail;	
	}
	else{
	    	list->head->next = node; 
		node->prev = list->head;
	}

	list->tail = node;

	list->size++;
	
	return 0;

}

dl_node *
dl_get_node(dl_list *list)
{
	if(!list)
		return NULL;

	dl_node *temp = list->head->next;

	dl_remove_node(list,temp);

	return temp;
}

int 
dl_empty(dl_list *list)
{
	return list->size == 0;
}

void
dl_print_list(dl_list *list)
{
	printf("Print list \n");
	if(!list)
		return;

	dl_node *n;

	for(n = list->head->next; n; n = n->next)
		printf("%x ",n->data);

	printf("\n");

	for(n = list->tail; n != list->head; n = n->prev)
		printf("%x ",n->data);

	printf("\nsize: %d\n",list->size);
	
}

dl_node *
dl_find_node(dl_list *list, void *data)
{
	if(!list || !data)
		return NULL;

	dl_node *n;

	for(n = list->head->next; n; n = n->next)
		if(n->data == data)
			return n;

	return NULL;

}

/*

dl_node *
GetHead(list *list)
{
	return list->head;
}

dl_node *
GetTail(list *list)
{
	return list->tail;
}






*/

