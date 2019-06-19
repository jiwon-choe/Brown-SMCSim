#ifndef LFQUEUE_H
#define LFQUEUE_H

#include "app_utils.hh"
#include "utils.hh"

#if 0
struct Node {
    ulong_t value;
    Node *next;
};
#endif

#define MAX_INDEX 2097152 //1048576
#define NEXT_INDEX(_x) ((_x+1) % MAX_INDEX)

class MyQueue
{
    private:
#if 0
        Node *head; // Node pointed to by head will always be sentinel.
                    // Real head will be head->next.
        Node *tail;
#endif
        ulong_t head_index;
        ulong_t tail_index;
        ulong_t queue_array[MAX_INDEX];

    public:
        MyQueue(); // constructor
        ~MyQueue(); // destructor
        void print();
        void enq(ulong_t val);
        ulong_t deq();
};


MyQueue::MyQueue()
{
#if 0
    head = new Node;
    head->value = 0;
    head->next = NULL;
    tail = head;
#endif
    head_index = 0;
    tail_index = 0;
    // head == tail means queue is empty
}

MyQueue::~MyQueue()
{
    // I don't use this anyway so..
}

void MyQueue::print()
{
#if 0
    Node * curr = head->next;
    while (curr != NULL) {
        cout << curr->value << " ";
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

void MyQueue::enq(ulong_t val)
{
    bool ret = false;
#if 0
    Node *last = NULL;
    Node *next = NULL;
    Node *new_node = new Node;
    new_node->next = NULL;
    new_node->value = val;

    while (true) {
        last = tail;
        next = tail->next;
        if (last == tail) {
            // tail didn't change in the meanwhile
            if (next == NULL) {
                // last is indeed seemingly the end of queue
                ret = __sync_bool_compare_and_swap(&(last->next), next, new_node);
                if (ret) {
                    __sync_bool_compare_and_swap(&tail, last, new_node);
                    break;
                }
            } else {
                // need to update tail
                __sync_bool_compare_and_swap(&tail, last, next);
            }
        }
    }
#endif
    ulong_t curr_index = 0;
    while (true) {
        curr_index = tail_index;
        ret = __sync_bool_compare_and_swap(&tail_index, curr_index, NEXT_INDEX(curr_index));
        if (ret) {
            // curr_index is reserved for this thread to insert data
            queue_array[curr_index] = val;
            break;
        }
    }
}

ulong_t MyQueue::deq()
{
#if 0
    Node *first = NULL;
    Node *last = NULL;
    Node *next = NULL;
    ulong_t value = 0;

    while (true) {
        first = head; // first is actually sentinel
        last = tail;
        next = first->next; // this next is the real head

        if (first == head) {
            // head didn't change in the meanwhile
            if (first == last) {
                // seems to be empty queue
                if (next == NULL) {
                    // really empty
                    return 0;
                } else {
                    // there is an item that is being enq'ed concurrently
                    __sync_bool_compare_and_swap(&tail, last, next);
                }
            } else {
                // nonempty queue
                value = next->value;
                if (__sync_bool_compare_and_swap(&head, first, next)) {
                    return value;
                }
            }
        }
    }
#endif
    ulong_t curr_index = 0;
    while (true) {
        curr_index = head_index;
        if (__sync_bool_compare_and_swap(&head_index, curr_index, NEXT_INDEX(curr_index))) {
            // curr_index is reserved for this thread to pop data
            return queue_array[curr_index];
        }
    }
}



#endif // ifndef LFQUEUE_H
