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
        int fc_push_operation(int thread_id, ulong_t parameter, SlotInfo *slot);
        int fc_pop_operation(int thread_id, SlotInfo *slot);
        void print();
    private:
        SlotInfo *enq_tail_slot;
        SlotInfo *deq_tail_slot;
        ulong_t enq_combine_cas;
        ulong_t deq_combine_cas;
        int volatile enq_operation_timestamp;
        int volatile deq_operation_timestamp;
        QNode *head_node;
        QNode *tail_node;
        void enq_enq_slot(SlotInfo *slot_to_enq);
        void enq_deq_slot(SlotInfo *slot_to_enq);
        void enq_flat_combine();
        void deq_flat_combine();
        ulong_t q_size;
};


Queue::Queue()
{
    tail_node = NULL;
    head_node = NULL;
    enq_combine_cas = 0;
    deq_combine_cas = 0;
    enq_tail_slot = NULL;
    deq_tail_slot = NULL;
    q_size = 0;
}


Queue::~Queue()
{
    QNode *curr = head_node;
    QNode *next = NULL;

    while (curr != NULL) {
        next = curr->next;
        delete curr;
        curr = next;
    }

}


void Queue::enq_enq_slot(SlotInfo *slot_to_enq)
{
    SlotInfo *curr_tail_slot = NULL;
    slot_to_enq->is_slot_linked = TRUE;
    slot_to_enq->op_timestamp = enq_operation_timestamp;

    do {
        curr_tail_slot  = enq_tail_slot;
        slot_to_enq->next_slot = curr_tail_slot;
    } while (__sync_bool_compare_and_swap(&enq_tail_slot, curr_tail_slot, slot_to_enq) == false);

}


void Queue::enq_deq_slot(SlotInfo *slot_to_enq)
{
    SlotInfo *curr_tail_slot = NULL;
    slot_to_enq->is_slot_linked = TRUE;
    slot_to_enq->op_timestamp = deq_operation_timestamp;

    do {
        curr_tail_slot  = deq_tail_slot;
        slot_to_enq->next_slot = curr_tail_slot;
    } while (__sync_bool_compare_and_swap(&deq_tail_slot, curr_tail_slot, slot_to_enq) == false);

}

void Queue::enq_flat_combine()
{
    SlotInfo *curr_slot = NULL;
    SlotInfo *prev_slot = NULL;
    int op = 0;
    ulong_t param = 0;

    curr_slot = enq_tail_slot;
    prev_slot = curr_slot;

    while (curr_slot != NULL) {
        if (curr_slot->is_request_valid == TRUE) {
            // execute operation
            param = curr_slot->parameter;
            curr_slot->ret_val = FALSE;

            QNode *new_node = new QNode(param);
            if (q_size++ == 0) { // if queue was empty
                head_node = new_node;
            }
            else {
                tail_node->next = new_node;
            }
            tail_node = new_node;
            curr_slot->ret_val = param;
#if 0
            else { // POP_OP
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

            }
#endif

            curr_slot->is_request_valid = FALSE;
            curr_slot->op_timestamp = enq_operation_timestamp;
            enq_operation_timestamp++;
        } else {
            // unlink slot if it has not been used for long
            if (curr_slot != prev_slot) { // if curr_slot is not tail_slot
                if ((enq_operation_timestamp - curr_slot->op_timestamp) > UNUSED_SLOT_THRESHOLD) {
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


void Queue::deq_flat_combine()
{
    SlotInfo *curr_slot = NULL;
    SlotInfo *prev_slot = NULL;
    int op = 0;
    ulong_t param = 0;
    QNode *old_head = head_node;

    curr_slot = deq_tail_slot;
    prev_slot = curr_slot;

    while (curr_slot != NULL) {
        if (curr_slot->is_request_valid == TRUE) {
            // execute operation
            curr_slot->ret_val = FALSE;
            /*if (q_size == 0) {
                curr_slot->ret_val = 0;
            }*/ // we're assuming that this never happens
            old_head = head_node;
            QNode *nxt = head_node->next;
            curr_slot->ret_val = head_node->val;
            head_node = nxt;
            delete old_head;

            curr_slot->is_request_valid = FALSE;
            curr_slot->op_timestamp = deq_operation_timestamp;
            deq_operation_timestamp++;
        } else {
            // unlink slot if it has not been used for long
            if (curr_slot != prev_slot) { // if curr_slot is not tail_slot
                if ((deq_operation_timestamp - curr_slot->op_timestamp) > UNUSED_SLOT_THRESHOLD) {
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


int Queue::fc_push_operation(int thread_id, ulong_t parameter, SlotInfo *slot)
{
    bool lock_acquired = false;
    slot->parameter = parameter;
    slot->is_request_valid = TRUE;

    if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
        enq_enq_slot(slot);
    }

    while (1) {
        lock_acquired = __sync_bool_compare_and_swap(&enq_combine_cas, 0, 0xffff); 

        if (lock_acquired) { // acquired lock
            if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
                enq_enq_slot(slot);
            }
            enq_flat_combine();
            enq_combine_cas = 0;
            return slot->ret_val;
        } else {
            while ((slot->is_request_valid == TRUE) && enq_combine_cas == 0xffff) {
                if (slot->is_slot_linked == FALSE) {
                    enq_enq_slot(slot);
                }
            }
            if (slot->is_request_valid == FALSE) {
                return slot->ret_val;
            }
        }
    }
}


int Queue::fc_pop_operation(int thread_id, SlotInfo *slot)
{
    bool lock_acquired = false;
    slot->is_request_valid = TRUE;

    if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
        enq_deq_slot(slot);
    }

    while (1) {
        lock_acquired = __sync_bool_compare_and_swap(&deq_combine_cas, 0, 0xffff); 

        if (lock_acquired) { // acquired lock
            if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
                enq_deq_slot(slot);
            }
            deq_flat_combine();
            deq_combine_cas = 0;
            return slot->ret_val;
        } else {
            while ((slot->is_request_valid == TRUE) && deq_combine_cas == 0xffff) {
                if (slot->is_slot_linked == FALSE) {
                    enq_deq_slot(slot);
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
    QNode *curr = head_node;

    cout << "linked list contents: " << endl;
    while (curr != NULL) {
        cout << curr->val << " ";
        curr = curr->next;
    }
    cout << "\n" << endl;
}


#endif // ifndef FCQUEUE_H
