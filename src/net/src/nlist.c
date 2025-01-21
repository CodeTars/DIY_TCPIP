#include "nlist.h"

void nlist_init(nlist_t *list) {
    list->first = list->last = (nlist_node_t *)0;
    list->count = 0;
}

void nlist_insert_first(nlist_t *list, nlist_node_t *node) {
    nlist_node_init(node);
    if (!nlist_is_empty(list)) {
        list->first->pre = node;
        node->next = list->first;
        list->first = node;
    } else {
        list->first = list->last = node;
    }
    list->count++;
}

void nlist_insert_last(nlist_t *list, nlist_node_t *node) {
    nlist_node_init(node);
    if (!nlist_is_empty(list)) {
        list->last->next = node;
        node->pre = list->last;
        list->last = node;
    } else {
        list->first = list->last = node;
    }
    list->count++;
}

nlist_node_t *nlist_remove(nlist_t *list, nlist_node_t *node) {
    if (node->pre) {
        node->pre->next = node->next;
    } else { // node is the first node
        list->first = node->next;
    }

    if (node->next) {
        node->next->pre = node->pre;
    } else { // node is the last node
        list->last = node->pre;
    }

    list->count--;
    nlist_node_init(node);
    return node;
}

void nlist_insert_after(nlist_t *list, nlist_node_t *node, nlist_node_t *new_node) {
    if (nlist_is_empty(list) || !node) {
        nlist_insert_first(list, new_node);
        return;
    }

    if (node->next) {
        node->next->pre = new_node;
    }
    new_node->next = node->next;

    node->next = new_node;
    new_node->pre = node;

    if (list->last == node) {
        list->last = new_node;
    }

    list->count++;
}