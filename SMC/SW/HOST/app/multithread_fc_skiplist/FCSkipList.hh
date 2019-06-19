#ifndef FCSKIPLIST_H
#define FCSKIPLIST_H

#include "app_utils.hh"  // ulong_t
#include "utils.hh"
#include <stdio.h>
#include <errno.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <time.h>

#define UNUSED_SLOT_THRESHOLD 10
#define LOCKED 1
#define UNLOCKED 0

#define MAX_LEVEL 14

struct Node {
    ulong_t key;       // 4 bytes
    ulong_t top_level; // 4 bytes
    Node *next[MAX_LEVEL]; // 4 * 14 = 56 bytes
}; // 64 bytes total

class SkipList {
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
        SkipList();   // constructor
        SkipList(ulong_t l, ulong_t u);   // constructor
        ~SkipList();  // destructor
        int fc_operation(int thread_id, int operation_type, ulong_t parameter, SlotInfo *slot);
        void print();
        bool in_bounds(ulong_t key);
        void set_max_level(ulong_t initsize);
    private:
        int max_level;
        ulong_t randomLevel();
        SlotInfo *tail_slot;
        ulong_t combine_cas;
        ulong_t ubound;
        ulong_t lbound;
        pthread_mutex_t tail_slot_lock; // used in place of atomic compare and swap operations
        int volatile operation_timestamp;
        Node *head_node;
        void enq_slot(SlotInfo *slot_to_enq);
        void flat_combine();
};


SkipList::SkipList():
    ubound(KEY_UPPER_BOUND),
    lbound(KEY_LOWER_BOUND)
{
    Node *sentinel_tail = new Node;
    sentinel_tail->key = INT_MAX;
    sentinel_tail->top_level = MAX_LEVEL;
    for (int i = 0; i < MAX_LEVEL; i++) {
        sentinel_tail->next[i] = NULL;
    }
    head_node = new Node;
    head_node->key = INT_MIN;
    head_node->top_level = MAX_LEVEL;
    for (int i = 0; i < MAX_LEVEL; i++) {
        head_node->next[i] = sentinel_tail;
    }
    combine_cas = 0;
    pthread_mutex_init(&tail_slot_lock, NULL);
    tail_slot = NULL;
    int size = ubound - lbound + 1;
    max_level = MAX_LEVEL;

    for (int i = 1; i < MAX_LEVEL; i++) {// max_level depends on size
        if (size < (1 << i) ) {
            max_level = i;
            break;
        } // max_level is roughly equal to log_2(size)
    }
}

SkipList::SkipList(ulong_t l, ulong_t u):
    ubound(u),
    lbound(l)
{
    if(ubound < lbound) {
        cout << "list made with bad bounds! exiting..." << endl;
        exit(1);
    }
    cout << "min bound: " << l << endl;
    cout << "max bound: " << u << endl;
    Node *sentinel_tail = new Node;
    sentinel_tail->key = INT_MAX;
    sentinel_tail->top_level = MAX_LEVEL;
    for (int i = 0; i < MAX_LEVEL; i++) {
        sentinel_tail->next[i] = NULL;
    }
    head_node = new Node;
    head_node->key = INT_MIN;
    head_node->top_level = MAX_LEVEL;
    for (int i = 0; i < MAX_LEVEL; i++) {
        head_node->next[i] = sentinel_tail;
    }
    combine_cas = 0;
    pthread_mutex_init(&tail_slot_lock, NULL);
    tail_slot = NULL;
    int size = ubound - lbound + 1;
    max_level = MAX_LEVEL;

    for (int i = 1; i < MAX_LEVEL; i++) {// max_level depends on size
        if (size < (1 << i) ) {
            max_level = i;
            break;
        } // max_level is roughly equal to log_2(size)
    }
}

SkipList::~SkipList()
{
    Node *currNode = head_node;
    Node *nextNode = NULL;

    while (currNode != NULL) {
        nextNode = currNode->next[0];
        delete currNode;
        currNode = nextNode;
    }

    pthread_mutex_destroy(&tail_slot_lock);
}


void SkipList::set_max_level(ulong_t initsize) {
    // added by JIWON: make max_level to be log_2(initsize)
    unsigned int i = 0;
    
    for (i = 1; i < MAX_LEVEL; i++) {
        if (initsize < (1 << i)) {
            max_level = i;
            break;
        }
    }
}

// Geometric distribution - stolen from Zhiyu code. Level n half as likely as n-1.

ulong_t SkipList::randomLevel() {
    int x = rand() % (1 << max_level);
    if ((x & 0x80000001) != 0) {// test highest and lowest bits
        return 1;
    }
    ulong_t level = 2;
    while (((x >>= 1) & 1) != 0)
        ++level;
    if(level > (max_level-1))
        return (max_level-1);
    return level;
}

bool SkipList::in_bounds(ulong_t key) {
    return (key >= lbound) && (key <= ubound);
}

/* Flat combining - enqueue operation in FC queue to
 * be processed by the lock owner
 */

void SkipList::enq_slot(SlotInfo *slot_to_enq)
{
    slot_to_enq->is_slot_linked = TRUE;
    slot_to_enq->op_timestamp = operation_timestamp;

    pthread_mutex_lock(&tail_slot_lock);
    slot_to_enq->next_slot = tail_slot;
    tail_slot = slot_to_enq;
    pthread_mutex_unlock(&tail_slot_lock);
}

// Go through and perform enqueued operations

void SkipList::flat_combine()
{
    SlotInfo *curr_slot = NULL;
    SlotInfo *prev_slot = NULL;
    int op = 0;
    ulong_t param = 0;
    Node *curr_node = NULL;
    Node *prev_node = head_node;
    Node *found_node = NULL;

    pthread_mutex_lock(&tail_slot_lock);
    curr_slot = tail_slot;
    pthread_mutex_unlock(&tail_slot_lock);
    prev_slot = curr_slot;
    Node *preds[MAX_LEVEL];
    Node *succs[MAX_LEVEL];

    while (curr_slot != NULL) {
        if (curr_slot->is_request_valid == TRUE) {
            // execute operation
            op = curr_slot->operation;
            param = curr_slot->parameter;
            curr_slot->ret_val = FALSE;

            prev_node = head_node;
            found_node = NULL;
            for (int iLevel = max_level - 1; iLevel >= 0; iLevel--) {

                curr_node = prev_node->next[iLevel];

                while (param > curr_node->key) {
                    prev_node = curr_node;
                    curr_node = prev_node->next[iLevel];
                }
                // at this point, prev_node->key < param <= curr_node->key
                if (NULL == found_node && param == curr_node->key) {
                    found_node = curr_node;
                    if (op == CONTAINS_OP) {
                        break;
                    }
                }
                preds[iLevel] = prev_node;
                succs[iLevel] = curr_node;
            }

            if (op == CONTAINS_OP) {
                if (found_node != NULL) {
                    curr_slot->ret_val = TRUE;
                }
            } else if (op == ADD_OP) {
                if (found_node == NULL) {
                    const int top_level = randomLevel();

                    // first link succs ........................................
                    // then link next fields of preds ..........................
                    Node* new_node = new Node;
                    new_node->key = param;
                    new_node->top_level = top_level;

                    for (int level = 0; level < top_level; ++level) {
                        new_node->next[level] = succs[level];
                        preds[level]->next[level] = new_node;
                    }
                    curr_slot->ret_val = TRUE;
                }
            } else { // REMOVE_OP
                if (found_node != NULL) {
                    for (int level = 0; level < found_node->top_level; ++level) {
                        preds[level]->next[level] = found_node->next[level];
                    }

                    delete found_node;
                    curr_slot->ret_val = TRUE;
                }
            }

            curr_slot->op_timestamp = operation_timestamp;
            curr_slot->is_request_valid = FALSE;
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


// Attempt to perform an operation - if thread gets the lock, then flat combine. Otherwise, enqueue.

int SkipList::fc_operation(int thread_id, int operation_type, ulong_t parameter, SlotInfo *slot)
{
    bool lock_acquired = false;
    slot->operation = operation_type;
    slot->parameter = parameter;
    slot->is_request_valid = TRUE;

    if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
        enq_slot(slot);
    }

    while (1) {
        lock_acquired = __sync_bool_compare_and_swap(&combine_cas, 0, 0xffff);

        if (lock_acquired) {
            if ((slot->is_slot_linked == FALSE) && (slot->is_request_valid == TRUE)) {
                enq_slot(slot);
            }
            flat_combine();
            combine_cas = 0;
            return slot->ret_val;
        } else {
            // Link operation in queue and spin until it returns
            while ((slot->is_request_valid == TRUE) && (combine_cas != 0)) {
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


void SkipList::print()
{
    ulong_t level = max_level - 1;
    Node *curr = NULL;

    cout << "skiplist contents: " << endl;
    while (1) {
        cout << "at level " << level << endl;
        curr = head_node->next[level];
        while (curr->next[level] != NULL) {
            cout << curr->key << endl;
            curr = curr->next[level];
        }
        if (level == 0) { break; }
        level--;
    }
}


#endif // ifndef FCSKIPLIST_H
