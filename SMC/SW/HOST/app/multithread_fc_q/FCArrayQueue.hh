#ifndef FCQUEUE_H
#define FCQUEUE_H

#include "app_utils.hh"  // ulong_t
#include "utils.hh"
#include <stdio.h>
#include <errno.h>
#include <algorithm>

#define UNUSED_SLOT_THRESHOLD 10
#define LOCKED 1
#define UNLOCKED 0

#if 0
struct QNode {
    QNode* volatile next;
    ulong_t val;

    QNode() :
            val(0), next(NULL) {
    }

    QNode(ulong_t v) :
            val(v), next(NULL) {
    }

    ~QNode() {
    }
};
#endif


#define MAX_INDEX 1048576
#define NEXT_INDEX(_x) ((_x+1) % MAX_INDEX)


class Queue {
    public:
        struct SlotInfo {
            char volatile is_request_valid;
            int volatile operation;
            ulong_t volatile parameter;
            int volatile ret_val;

            int volatile op_timestamp;
            char volatile is_slot_linked;
            SlotInfo *volatile next_slot;

            SlotInfo() {
                is_request_valid = FALSE;
                operation = 0;
                parameter = 0;
                ret_val = 0;
                op_timestamp = 0;
                is_slot_linked = FALSE;
                next_slot = NULL;
            }
        };
        Queue();   // constructor
        ~Queue();  // destructor
        int fc_operation(int thread_id, int operation_type, ulong_t parameter, SlotInfo *slot);
        void print();
    private:
        SlotInfo *tail_slot;
        //pthread_mutex_t combine_lock;
        ulong_t combine_cas;
        //int combine_lock_status;
        pthread_mutex_t tail_slot_lock; // used in place of atomic compare and swap operations
        int volatile operation_timestamp;
#if 0
        QNode *head_node;
        QNode *tail_node;
#endif
        ulong_t head_index;
        ulong_t tail_index;
        ulong_t queue_array[MAX_INDEX];
        void enq_slot(SlotInfo *slot_to_enq);
        void flat_combine();
        ulong_t q_size;
};


Queue::Queue()
{
#if 0
    tail_node = NULL;
    head_node = NULL;
#endif
    head_index = 0;
    tail_index = 0;
    //pthread_mutex_init(&combine_lock, NULL);
    //combine_lock_status = UNLOCKED;
    combine_cas = 0;
    pthread_mutex_init(&tail_slot_lock, NULL);
    tail_slot = NULL;
    q_size = 0;
}


Queue::~Queue()
{
#if 0
    QNode *curr = head_node;
    QNode *next = NULL;

    while (curr != NULL) {
        next = curr->next;
        delete curr;
        curr = next;
    }
#endif

    //pthread_mutex_destroy(&combine_lock);
    pthread_mutex_destroy(&tail_slot_lock);
}


void Queue::enq_slot(SlotInfo *slot_to_enq)
{
    SlotInfo *curr_tail_slot = NULL;
    slot_to_enq->is_slot_linked = TRUE;
    slot_to_enq->op_timestamp = operation_timestamp;

    do {
        curr_tail_slot  = tail_slot;
        slot_to_enq->next_slot = curr_tail_slot;
    } while (__sync_bool_compare_and_swap(&tail_slot, curr_tail_slot, slot_to_enq) == false);

#if 0
    pthread_mutex_lock(&tail_slot_lock);
    slot_to_enq->next_slot = tail_slot;
    tail_slot = slot_to_enq;
    pthread_mutex_unlock(&tail_slot_lock);
#endif
}

void Queue::flat_combine()
{
    SlotInfo *curr_slot = NULL;
    SlotInfo *prev_slot = NULL;
    int op = 0;
    ulong_t param = 0;
//    QNode *old_head = head_node;

    pthread_mutex_lock(&tail_slot_lock);
    curr_slot = tail_slot;
    pthread_mutex_unlock(&tail_slot_lock);
    prev_slot = curr_slot;

    while (curr_slot != NULL) {
        if (curr_slot->is_request_valid == TRUE) {
            // execute operation
            op = curr_slot->operation;
            param = curr_slot->parameter;
            curr_slot->ret_val = FALSE;

            if (op == PUSH_OP) {
#if 0
                QNode *new_node = new QNode(param);
                if (q_size++ == 0) { // if queue was empty
                    head_node = new_node;
                }
                else {
                    tail_node->next = new_node;
                }
                tail_node = new_node;
                curr_slot->ret_val = param;
#endif
                queue_array[tail_index] = param;
                tail_index = NEXT_INDEX(tail_index);
                curr_slot->ret_val = param;
            }
            else { // POP_OP
#if 0
                if (q_size == 0) {
                    curr_slot->ret_val = 0;
                }
                else {
                    old_head = head_node;
                    QNode *nxt = head_node->next;
                    curr_slot->ret_val = head_node->val;
                    head_node = nxt;
                    if (--q_size == 0) {
                        tail_node = NULL;
                    }
                    delete old_head;
                }
#endif
                curr_slot->ret_val = queue_array[head_index];
                head_index = NEXT_INDEX(head_index);
            }

            curr_slot->is_request_valid = FALSE;
            curr_slot->op_timestamp = operation_timestamp;
            operation_timestamp++;
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
}

int Queue::fc_operation(int thread_id, int operation_type, ulong_t parameter, SlotInfo *slot)
{
    //int lock_acquired = 0;
    bool lock_acquired = false;
    slot->operation = operation_type;
    slot->parameter = parameter;
    slot->is_request_valid = TRUE;

    if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
        enq_slot(slot);
    }

    while (1) {
        //lock_acquired = pthread_mutex_trylock(&combine_lock);
        lock_acquired = __sync_bool_compare_and_swap(&combine_cas, 0, 0xffff); 

        //if (lock_acquired == 0) { // acquired lock
        if (lock_acquired) { // acquired lock
            //combine_lock_status = LOCKED;
            if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
                enq_slot(slot);
            }
            flat_combine();
            //combine_lock_status = UNLOCKED;
            //pthread_mutex_unlock(&combine_lock);
            combine_cas = 0;
            return slot->ret_val;
        } else {
#ifdef HOST_DEBUG_ON
            /*if (lock_acquired != EBUSY) {
                perror("pthread_mutex_trylock");
            }*/
#endif
            //while ((slot->is_request_valid == TRUE) && combine_lock_status == LOCKED) {
            while ((slot->is_request_valid == TRUE) && combine_cas == 0xffff) {
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


void Queue::print()
{
#if 0
    QNode *curr = head_node;

    cout << "linked list contents: " << endl;
    while (curr != NULL) {
        cout << curr->val << " ";
        curr = curr->next;
    }
    cout << "\n" << endl;
#endif
    ulong_t i = head_index;
    while (i != tail_index) {
        cout << queue_array[i] << " ";
        i = NEXT_INDEX(i);
    }
    cout << "\n" << endl;
}


#endif // ifndef FCQUEUE_H
