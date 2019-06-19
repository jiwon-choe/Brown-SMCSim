//Lazy Fine-Grained Lock
#ifndef LazyLockLinkedList_h
#define LazyLockLinkedList_h
#include <stdio.h>
#include <iostream>
#include "app_utils.hh"
#include "defs.hh"

using namespace std;


class LinkedList{
public:
    LinkedList(node_t *list_head, node_t *list_tail); // constructor 
    ~LinkedList();            // Destructor
    int contains(ulong_t x); // Returns true if x is in the list, otherwise returns false
    int add(ulong_t x);      // Returns true when successfully added, otherwise returns false
    int remove(ulong_t x);   // Returns true when successfully removed, otherwise returns false

private:
    node_t *head; 
    node_t *tail;
};

//**********************    constructor    **************************
LinkedList::LinkedList(node_t *list_head, node_t *list_tail)
{
    head = list_head;
    tail = list_tail;
}

//**********************    destructor     **************************
LinkedList::~LinkedList()
{
    node_t *prev = head;
    node_t *curr = prev->next;

    while (curr->next != NULL) {
        if (curr->marked == true) {
            prev->next = curr->next;
        }
        prev = curr; 
        curr = prev->next;
    }
}

//*******************    contain() function    ***********************
int LinkedList::contains(ulong_t x){
    node_t *curr_node = head;

    while (curr_node->key < x) {
        curr_node = curr_node->next;
    }

    if ((curr_node->key == x) && (curr_node->marked == false)) {
        return 1;
    } else {
        return 0;
    }
}


//*******************    add() function    ***********************
int LinkedList::add(ulong_t x){
    int retval = 0;
    node_t *prev_node = head;
    node_t *curr_node = head->next;

    while (true) {
        prev_node = head;
        curr_node = head->next;

        while (curr_node->key < x) {
            prev_node = curr_node;
            curr_node = curr_node->next;
        }

        if (pthread_mutex_lock(&prev_node->node_lock) == 0) {
            if (pthread_mutex_lock(&curr_node->node_lock) == 0) {
                if (!prev_node->marked && !curr_node->marked && (prev_node->next == curr_node)) {
                    // both prev and curr nodes are still valid nodes in list
                    if (curr_node->key != x) {
                        node_t *new_node = (node_t *)allocate_region(sizeof(node_t));
                        new_node->key = x;
                        new_node->next = curr_node;
                        pthread_mutex_init(&new_node->node_lock, NULL);
                        prev_node->next = new_node;
                        retval = 1;
                    }
                    pthread_mutex_unlock(&curr_node->node_lock);
                    pthread_mutex_unlock(&prev_node->node_lock);
                    return retval;
                }
                pthread_mutex_unlock(&curr_node->node_lock);
            }
            pthread_mutex_unlock(&prev_node->node_lock);
        }
    }
}


//*******************    remove() function    ***********************
int LinkedList::remove(ulong_t x){
    node_t *prev_node = head;
    node_t *curr_node = head->next;

    while (true) {
        prev_node = head;
        curr_node = head->next;

        while (curr_node->key < x) {
            prev_node = curr_node;
            curr_node = curr_node->next;
        }

        if (pthread_mutex_lock(&prev_node->node_lock) == 0) {
            if (pthread_mutex_lock(&curr_node->node_lock) == 0) {
                if (!prev_node->marked && !curr_node->marked && (prev_node->next == curr_node)) {
                    // both prev and curr nodes are still valid nodes in list
                    if (curr_node->key == x) {
                        curr_node->marked = true;
                        prev_node->next = curr_node->next;
                        pthread_mutex_unlock(&curr_node->node_lock);
                        //pthread_mutex_destroy(&curr_node->node_lock);
                        pthread_mutex_unlock(&prev_node->node_lock); 
                        //delete curr_node;
                        return 1;
                    } else {
                        pthread_mutex_unlock(&curr_node->node_lock);
                        pthread_mutex_unlock(&prev_node->node_lock);
                        return 0;
                    }
                    
                }
                pthread_mutex_unlock(&curr_node->node_lock);
            }
            pthread_mutex_unlock(&prev_node->node_lock);
        }
    }
}



#endif
