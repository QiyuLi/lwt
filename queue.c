#include <stdio.h>
#include <stdlib.h>

#include <queue.h>

q_node *
q_make_node(void *data)
{
	q_node *n = malloc(sizeof(q_node));
	
	n->data = data;
	n->next = NULL;

	return n;
}

queue *
q_init(void)
{
	queue *q = malloc(sizeof(queue));

	q->size = 0;

	q->head = q_make_node(NULL);

	q->tail = q->head;

	return q;
}


void
q_clear(queue *q)
{
	if(!q)
		return;

	q->size = 0;

	q_node *n = q->head->next;

	q_node *temp;

	while(n){	
		temp = n;
		n = n->next;
		free(temp);
	}		
}


void 
q_free(queue *q)
{
	if(!q)
		return;

	q_clear(q);
	
	free(q->head);
	free(q);
}

int 
q_enqueue(queue *q, q_node *node)
{
	if(!q)
		return -1;

	q->tail->next = node;
	q->tail = node;
	q->tail->next = NULL;
	q->size++;	

	return 0;
}

int 
q_enqueue_sync(queue *q, q_node *node)
{
	q_node *old_val, *new_val;

	new_val = node;

	do{
		if(!q)
			return -1;

		old_val = q->tail;
		
	}while(!__sync_bool_compare_and_swap(&(q->tail),old_val,new_val));
	

	old_val->next = new_val;

	__sync_fetch_and_add(&q->size, 1);	

	return 0;

}


q_node *
q_dequeue(queue *q)
{
	if(!q || !q->size)
		return NULL;

	q_node *n = q->head->next;

	q->head->next = n->next;

	q->size--;

	if(n == q->tail)
		q->tail = q->head;
	
	return n;
}

q_node *
q_dequeue_sync(queue *q)
{
	q_node *old_val, *new_val;

	do{
		if(!q)
			return -1;

		old_val = q->head->next;
		new_val = old_val->next;

	}while(!__sync_bool_compare_and_swap(&(q->head->next),old_val,new_val));
	
	if(old_val == q->tail)
		q->tail = q->head;

	__sync_fetch_and_add(&q->size, -1);

	return old_val;
}

int
q_remove(queue *q, q_node *node)
{
	if(!q || !q->size)
		return -1;

	q_node *n, *temp;

	for(n = q->head, temp = n->next; temp; n = n->next, temp = temp->next)		
		if(temp == node){
			n->next = temp->next;
			q->size--;
			
			if(temp == q->tail)
				q->tail = n;

			return 0;	
		}

	return -1;
}


int 
q_remove_sync(queue *q, q_node *node)
{
	return -1;
}

void
q_print(queue *q)
{
	if(!q)
		return;

	q_node *n;

	for(n = q->head->next; n; n = n->next)
		printf("%x %d \n",n, n->data);

}

int 
q_empty(queue *q)
{
	if(q)
		return q->size == 0;
	else
		return 0;
}


q_node *
q_find_node(queue *q, void *data)
{
	if(!q)
		return NULL;

	q_node *n;

	for(n = q->head->next; n; n = n->next)
		if(n->data == data)
			return n;
	return NULL;
}

