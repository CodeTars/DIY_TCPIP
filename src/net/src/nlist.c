#include "nlist.h"

void nlist_init(nlist_t *list)
{
    list->first = list->last = (nlist_node_t *)0;
    list->count = 0;
}

void nlist_insert_first(nlist_t *list, nlist_node_t *node)
{
    nlist_node_init(node);
    if (!nlist_is_empty(list))
    {
        list->first->pre = node;
        node->next = list->first;
        list->first = node;
    }
    else
    {
        list->first = list->last = node;
    }
    list->count++;
}