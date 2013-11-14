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
	q->head = malloc(sizeof(q_node));
	q->tail = q->head;

	return q;
}


void
q_clear(queue *q)
{
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
q_destroy(queue *q)
{
	q_clear(q);

	free(q);
}

int 
q_enqueue(queue *q, void *data)
{
	q_node *n = q_make_node(data);

	q->tail->next = n;
	q->tail = n;

	q->size++;	

	return 0;
}

void *
q_dequeue(queue *q)
{
	if(!q->size)
		return NULL;

	q_node *n = q->head->next;

	q->head->next = n->next;

	void *data = n->data;

	free(n);

	q->size--;
	
	return data;
}



int 
q_remove(queue *q, void *data)
{
	q_node *n, *temp;

	for(n = q->head, temp = n->next; n && temp; n = n->next, temp = temp->next)		
		if(temp->data == data){
			n->next = temp->next;
			free(temp);
			q->size--;
			return 0;	
		}

	return -1;
}

int 
q_enqueue_sync(queue *q, void *data)
{
	q_node *n = q_make_node(data);
	q_node *old_val, *new_val;

	do{
		old_val = q->tail;

	}while(!__sync_bool_compare_and_swap(&q->tail,old_val,new_val));
	

	old_val->next = new_val;

	q->size++;	

	return 0;

}

void *
q_dequeue_sync(queue *q)
{
	return NULL;
}

int 
q_remove_sync(queue *q, void *data)
{
	return -1;
}

void
q_print(queue *q)
{
	q_node *n;

	for(n = q->head->next; n; n = n->next)
		printf("%d ",n->data);

	printf("\n");
}

int 
q_empty(queue *q)
{
	return q->size == 0;
}


q_node *
q_find_node(queue *q, void *data)
{
	q_node *n;

	for(n = q->head->next; n; n = n->next)
		if(n->data == data)
			return n;
	return NULL;
}

/*

int main()
{
	queue * q = q_init();

	int i;
	
	for(i = 1; i < 5; i++)
		q_enqueue(q,i);

	q_remove(q,3);

	for(i = 1; i < 5; i++)
		printf("%d \n",q_dequeue(q));


	return 0;
}
*/

