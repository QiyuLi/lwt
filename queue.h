#ifndef QUEUE_H
#define QUEUE_H

typedef struct q_node
{
	void *data;
	struct q_node *next;
} q_node;

typedef struct queue
{
	q_node *head;
	q_node *tail;
	int size
} queue;

q_node *
q_make_node(void *data);

queue *
q_init(void);

void 
q_destroy(queue *q);

void
q_clear(queue *q);

int 
q_enqueue(queue *q, void *data);

void *
q_dequeue(queue *q);

int 
q_remove(queue *q, void *data);


int 
q_enqueue_sync(queue *q, void *data);

void *
q_dequeue_sync(queue *q);

int 
q_remove_sync(queue *q, void *data);

int 
q_empty(queue *q);

void
q_print(queue *q);

q_node *
q_find_node(queue *q, void *data);



#endif
