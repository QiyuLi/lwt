#ifndef D_LINKED_LIST_H
#define D_LINKED_LIST_H

typedef struct dl_node
{
	void *data;
	struct dl_node *prev;
	struct dl_node *next;
} dl_node;

typedef struct dl_list
{
	dl_node *head;
	dl_node *tail;
	int size
} dl_list;


dl_node *
dl_make_node(void *data);

dl_list *
dl_init_list(void);

void 
dl_destroy_list(dl_list *list);

void
dl_clear_list(dl_list *list);

int 
dl_remove_node(dl_list *list, dl_node *node);

int 
dl_add_node(dl_list *list, dl_node *node);

dl_node *
dl_get_node(dl_list *list);

int 
dl_empty(dl_list *list);

void
dl_print_list(dl_list *list);

dl_node *
dl_find_node(dl_list *list, void *data);


#endif
