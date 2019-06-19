#ifndef FCLINKEDLIST_H
#define FCLINKEDLIST_H

#include "app_utils.hh"
#include "defs.hh"
#include <algorithm>

#define LOCKED 1
#define UNLOCKED 0
#define UNUSED_SLOT_THRESHOLD 15

class LinkedList {

    public:
        struct SlotInfo {
            char volatile is_request_valid; // 1 byte 
            int volatile operation;         // 4 bytes
            ulong_t volatile parameter;     // 4 bytes
            int volatile ret_val;           // 4 bytes
            int volatile op_timestamp;      // 4 bytes

            char volatile is_slot_linked;   // 1 byte
            SlotInfo *volatile next_slot;   // 4 bytes
            char padding[230];              // TODO?: remove padding??

            SlotInfo() {                    // 4 bytes
                is_request_valid = FALSE;
                operation = 0;
                parameter = 0;
                ret_val = 0;
                op_timestamp = 0;
                is_slot_linked = FALSE;
                next_slot = NULL;
            }
        }; 

        LinkedList(node_t *head, node_t *tail); // constructor
        ~LinkedList(); // destructor

        int fc_operation(int thread_id, int operation_type, ulong_t parameter, SlotInfo *slot);

    private:
        SlotInfo *tail_slot;
        pthread_mutex_t combine_lock;
        int combine_lock_status; 
        pthread_mutex_t tail_slot_lock;
        int volatile operation_timestamp;

        node_t *head_node;
        node_t *tail_node;

        void enq_slot(SlotInfo *slot_to_enq);
        void flat_combine_sort();
};


LinkedList::LinkedList(node_t *head, node_t *tail) 
{
    head_node = head;
    tail_node = tail;

    pthread_mutex_init(&combine_lock, NULL);
    combine_lock_status = UNLOCKED;
    pthread_mutex_init(&tail_slot_lock, NULL);
    tail_slot = NULL;
    operation_timestamp = 0;
}


LinkedList::~LinkedList()
{
    pthread_mutex_destroy(&combine_lock);
    pthread_mutex_destroy(&tail_slot_lock);
}


void LinkedList::enq_slot(SlotInfo *slot_to_enq)
{
    slot_to_enq->is_slot_linked = TRUE;
    slot_to_enq->op_timestamp = operation_timestamp;
    
    pthread_mutex_lock(&tail_slot_lock);
    slot_to_enq->next_slot = tail_slot;
    tail_slot = slot_to_enq;
    pthread_mutex_unlock(&tail_slot_lock);
}


static bool requests_compare(LinkedList::SlotInfo *slot_a, LinkedList::SlotInfo *slot_b)
{
    return (slot_a->parameter < slot_b->parameter);
};


void LinkedList::flat_combine_sort()
{
    SlotInfo *curr_slot = NULL;
    SlotInfo *prev_slot = NULL;
    node_t *curr_node = head_node;
    node_t *prev_node = NULL;
    node_t *new_node = NULL;
    int op = 0;
    ulong_t param = 0;

    vector<SlotInfo *> requests;

    pthread_mutex_lock(&tail_slot_lock);
    curr_slot = tail_slot;
    pthread_mutex_unlock(&tail_slot_lock);
    prev_slot = curr_slot;

    while (curr_slot != NULL) {
        if (curr_slot->is_request_valid == TRUE) {
            requests.push_back(curr_slot);
        } else {
            // unlink slot if it has not been used for long
            if (curr_slot != prev_slot) { // if curr_slot is not tail_slot
                if ((operation_timestamp - curr_slot->op_timestamp) > UNUSED_SLOT_THRESHOLD) {
                    prev_slot->next_slot = curr_slot->next_slot;
                    curr_slot->is_slot_linked = FALSE;
                    curr_slot = prev_slot->next_slot;
                    continue;
                }
            }
        }
        prev_slot = curr_slot;
        curr_slot = curr_slot->next_slot;
    }

    // sort requests
    sort(requests.begin(), requests.end(), requests_compare);

    // execute requests
    for (vector<SlotInfo *>::iterator iter = requests.begin(); iter != requests.end(); ++iter) {
        op = (*iter)->operation;
        param = (*iter)->parameter;
        (*iter)->ret_val = FALSE;

        while (curr_node->key < param) {
            prev_node = curr_node;
            curr_node = curr_node->next;
        }

        if (op == CONTAINS_OP) {
            if (curr_node->key == param) {
                (*iter)->ret_val = TRUE;
            }
        } else if (op == ADD_OP) {
            if (curr_node->key != param) {
                new_node = (node_t *)allocate_region(sizeof(node_t));
                new_node->key = param;
                new_node->next = curr_node;
                prev_node->next = new_node;
                curr_node = new_node;
                (*iter)->ret_val = TRUE;
            }
        } else { // REMOVE_OP
            if (curr_node->key == param) {
                prev_node->next = curr_node->next;
                curr_node = prev_node->next;
                (*iter)->ret_val = TRUE;
            }
        }
        (*iter)->op_timestamp = operation_timestamp;
        (*iter)->is_request_valid = FALSE;
        operation_timestamp++;
    }
}


int LinkedList::fc_operation(int thread_id, int operation_type, ulong_t parameter, SlotInfo *slot) 
{
    int lock_acquired = 0;
    slot->operation = operation_type;
    slot->parameter = parameter;
    slot->is_request_valid = TRUE;

    if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
        enq_slot(slot);
    }

    while (1) {
        lock_acquired = pthread_mutex_trylock(&combine_lock);
        
        if (lock_acquired == 0) { // acquired lock
            combine_lock_status = LOCKED;
            if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
                enq_slot(slot);
            }
            flat_combine_sort();
            combine_lock_status = UNLOCKED;
            pthread_mutex_unlock(&combine_lock);
            return slot->ret_val;
        } else {
/*#ifdef HOST_DEBUG_ON
            cout << "T" << thread_id << " trylock fail with: " << lock_acquired << endl;
            if (lock_acquired != EBUSY) {
                perror("pthread_mutex_trylock");
            }
#endif*/
            while ((slot->is_request_valid == TRUE) && combine_lock_status == LOCKED) {
                if (slot->is_slot_linked == FALSE) {
                    enq_slot(slot);
                }
            }
            if (slot->is_request_valid == FALSE) {
                return slot->ret_val;
            }
        }
    }
}

#endif // FCLLINKEDLIST_H
