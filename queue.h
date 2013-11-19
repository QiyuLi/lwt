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
q_free(queue *q);

void
q_clear(queue *q);

int 
q_enqueue(queue *q, q_node *node);

int 
q_enqueue_sync(queue *q, q_node *node);

q_node *
q_dequeue(queue *q);

q_node *
q_dequeue_sync(queue *q);

int 
q_remove(queue *q, q_node *node);

int 
q_remove_sync(queue *q, q_node *node);

int 
q_empty(queue *q);

void
q_print(queue *q);

q_node *
q_find_node(queue *q, void *data);



#endif
