#ifndef DList_H
#define DList_H

typedef struct Node
{
	void *data;
	struct Node *prev;
	struct Node *next;
} Node;

typedef struct DList
{
	Node *head;
	Node *tail;
	int size
} DList;



Node *
MakeNode(void *data);

DList *
InitDList(void);

void 
DestroyDList(DList *dlist);


void
ClearDList(DList *dlist);

Node *
GetHead(DList *dlist);

Node *
GetTail(DList *dlist);

int 
RemoveNode(DList *dlist, Node *node);

int 
AddNode(DList *dlist, Node *node);

Node *
GetNode(DList *dlist);

Node *
FindNode(DList *dlist, void *data);

void
PrintDList(DList *dlist);

#endif
