#include <stdio.h>
#include <stdlib.h>
#include <DList.h>


Node *
MakeNode(void *data)
{
	Node *n = malloc(sizeof(Node));
	
	n->data = data;
	n->next = n->prev = NULL;

	return n;
}

DList *
InitDList()
{
	DList *dlist = malloc(sizeof(DList));	
	
	dlist->head = dlist->tail = MakeNode(NULL);

	dlist->size = 0;

	return dlist;
}

void 
DestroyDList(DList *dlist)
{
	ClearDList(dlist);

	free(dlist->head);

	free(dlist);
}

void
ClearDList(DList *dlist)
{
	Node *n = dlist->tail;
	Node *temp;

	while(dlist->size > 0){	
		temp = n;
		n = n->prev;
		free(temp);
		dlist->size--;
	}
	
	dlist->tail = dlist->head;
}

Node *
GetHead(DList *dlist)
{
	return dlist->head;
}

Node *
GetTail(DList *dlist)
{
	return dlist->tail;
}

int 
RemoveNode(DList *dlist, Node *node)
{
	if(!dlist || !node || !node->prev)
		return -1;
	
	node->prev->next = node->next;

	if(node->next)
		node->next->prev = node->prev;
	else
		dlist->tail = node->prev;

	node->next = node->prev = NULL;

	dlist->size--;
	
	return 0;
}

int 
AddNode(DList *dlist, Node *node)
{
	if(!dlist || !node)
		return -1;
	
	if(dlist->tail != dlist->head){
	    	dlist->tail->next = node;
		node->prev = dlist->tail;	
	}
	else{
	    	dlist->head->next = node; 
		node->prev = dlist->head;
	}

	dlist->tail = node;

	dlist->size++;
	
	return 0;

}

Node *
GetNode(DList *dlist)
{
	if(!dlist)
		return NULL;

	Node *temp = dlist->head->next;

	RemoveNode(dlist,temp);

	return temp;
}

Node *
FindNode(DList *dlist, void *data)
{
	if(!dlist || !data)
		return NULL;

	Node *n;

	for(n = dlist->head->next; n; n = n->next)
		if(n->data == data)
			return n;

	return NULL;

}

void
PrintDList(DList *dlist)
{
	printf("Print DList \n");
	if(!dlist)
		return;

	Node *n;

	for(n = dlist->head->next; n; n = n->next)
		printf("%x ",n->data);

	printf("\n");

	for(n = dlist->tail; n != dlist->head; n = n->prev)
		printf("%x ",n->data);

	printf("\nsize: %d\n",dlist->size);
	
}


