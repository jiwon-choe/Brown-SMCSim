#ifndef LFQUEUE_H
#define LFQUEUE_H

#include "app_utils.hh"
#include "utils.hh"

struct Node {
    ulong_t value;
    Node *next;
};

class MyQueue
{
    private:
        Node *head; // Node pointed to by head will always be sentinel.
                    // Real head will be head->next.
        Node *tail;

    public:
        MyQueue(); // constructor
        ~MyQueue(); // destructor
        void print();
        void enq(ulong_t val);
        ulong_t deq();
};


MyQueue::MyQueue()
{
    head = new Node;
    head->value = 0;
    head->next = NULL;
    tail = head;
}

MyQueue::~MyQueue()
{
    // I don't use this anyway so..
}

void MyQueue::print()
{
    Node * curr = head->next;
    while (curr != NULL) {
        cout << curr->value << " ";
        curr = curr->next;
    }
    cout << "\n" << endl;
}

void MyQueue::enq(ulong_t val)
{
    bool ret = false;
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
}

ulong_t MyQueue::deq()
{
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
}



#endif // ifndef LFQUEUE_H
