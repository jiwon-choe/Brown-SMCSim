#ifndef UTILS_H_MAINMEM_PIM_LINKEDLIST
#define UTILS_H_MAINMEM_PIM_LINKEDLIST

#include "_app_params.h"
#include "defs.hh"


typedef struct skiplist_node {
    ulong_t key;
#ifdef HOST_LAZY_LOCK
    volatile bool marked;
    pthread_mutex_t node_lock;
#endif
    struct skiplist_node *next[MAX_LEVEL];
    ulong_t top_level;
} skiplist_node_t;


int list_add(skiplist_node_t *start_node, ulong_t key, ulong_t new_level) {
    skiplist_node_t *preds[MAX_LEVEL] = {0};
    skiplist_node_t *succs[MAX_LEVEL] = {0};
    skiplist_node_t *new_node = NULL;
    skiplist_node_t *curr_node = NULL;
    skiplist_node_t *prev_node = start_node;
    int level = 0;

    for (level = MAX_LEVEL-1; level >= 0; level--) {
        curr_node = prev_node->next[level];
        while (key > curr_node->key) {
            prev_node = curr_node;
            curr_node = prev_node->next[level];
        }
        if (curr_node->key == key) {
            return 0;
        }
        preds[level] = prev_node;
        succs[level] = curr_node;
    }

    new_node = (skiplist_node_t *)allocate_region(sizeof(skiplist_node_t));
#ifdef HOST_LAZY_LOCK
    pthread_mutex_init(&new_node->node_lock, NULL);
    new_node->marked = false;
#endif
    new_node->top_level = new_level;
    new_node->key = key;
    for (level = 0; level < (int)(new_node->top_level); level++) {
        new_node->next[level] = succs[level];
        (preds[level])->next[level] = new_node;
    }
    return 1;
}

void list_print(node_t *start) {
    node_t *curr = start->next;
    while (curr->next != 0) {
        cout << curr->key << endl;
        curr = curr->next;
    }
}
#endif
