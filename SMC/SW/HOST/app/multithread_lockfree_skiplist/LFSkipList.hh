#ifndef LFSKIPLIST_H
#define LFSKIPLIST_H

#include "app_utils.hh"  // ulong_t
#include "utils.hh"
#include <stdio.h>
#include <errno.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <time.h>

#define MARK_BIT 0x1

// ops for getting flag bits - pointers should be 8 byte aligned so last few bits can be used for flag bit.

// Get the address of the node (stripping flags)
#define PTR(d) ((Node *) (((unsigned long) (d)) & 0xffffffffffffff8))

// logically a boolean indicating 1 if marked, 0 if not
#define MARKED(d) (((unsigned long) (d)) & MARK_BIT)

// apply mark operation to a pointer
#define MARK(d) ((Node *) (((unsigned long) (d)) | MARK_BIT))

#define MAX_LEVEL 30

struct Node {
    ulong_t key;       // 4 bytes
    ulong_t top_level;
    Node *next[MAX_LEVEL];
};

class SkipList {
    public:
        SkipList();   // constructor
        SkipList(ulong_t l, ulong_t u);   // constructor
        ~SkipList();  // destructor
        void print();
        void set_max_level(ulong_t initsize);
        bool add(ulong_t x);
        bool contains(ulong_t x);
        bool remove(ulong_t x);
    private:
        Node *head;
        Node *tail;
        int max_level;
        ulong_t randomLevel();
        Node *alloc_node();
        ulong_t next_node_addr;
        void findAndClean(ulong_t key);
        Node* find(ulong_t key, Node** preds, Node** succs);
};

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

void SkipList::findAndClean(ulong_t key) {
    bool marked = false;
    bool snip;
    Node* pPred;
    Node* pCurr;
    Node* pSucc;
    Node* pPredFirst;

find_retry:
    int iLevel;
    while (true) {
        pPredFirst = head;
        for (iLevel = max_level - 1; iLevel >= 0; --iLevel) {
            pPred = pPredFirst;
            pCurr = pPred->next[iLevel];
            while (true) {
                pSucc = pCurr->next[iLevel];
                // The mark for the current list item lies in the next pointer.
                marked = MARKED(pSucc);
                pSucc = PTR(pSucc);
                while (marked) {           // replace curr if marked, and repeat if next is also marked, and so on
                    snip = __sync_bool_compare_and_swap(&pPred->next[iLevel], PTR(pCurr), PTR(pSucc));
                    if (!snip) {
                        // If this occurs, it's because another thread did it before this one could. Better to back off
                        // entirely and retry. 
                        goto find_retry;
                    }

                    pCurr = PTR(pPred->next[iLevel]);
                    pSucc = pCurr->next[iLevel];
                    marked = MARKED(pSucc);
                    pSucc = PTR(pSucc);
                }
                if (pCurr->key < key) { // move forward same iLevel
                    pPred = pCurr;
                    pPredFirst = pPred;
                    pCurr = pSucc;
                }
                else if (pCurr->key == key) {
                    pPred = pCurr;
                    pCurr = pSucc;
                }
                else {
                    break; // move to next iLevel
                }
            }

        }

        return;
    }
}

Node* SkipList::find(ulong_t key, Node** preds, Node** succs) {
    bool marked = false;
    bool snip;
    Node* pPred;
    Node* pCurr;
    Node* pSucc;

find_retry:
    int iLevel;
    while (true) {
        pPred = head;
        for (iLevel = max_level-1; iLevel >= 0; --iLevel) {
            pCurr = PTR(pPred->next[iLevel]);
            while (true) {
                pSucc = pCurr->next[iLevel];
                marked = MARKED(pSucc);
                pSucc = PTR(pSucc);
                while (marked) {           // replace curr if marked
                    snip = __sync_bool_compare_and_swap(&pPred->next[iLevel], PTR(pCurr), PTR(pSucc));
                    if (!snip) {
                        goto find_retry;
                    }

                    pCurr = PTR(pPred->next[iLevel]);
                    pSucc = pCurr->next[iLevel];
                    marked = MARKED(pSucc);
                    pSucc = PTR(pSucc);
                }
                if (pCurr->key < key) { // move forward same iLevel
                    pPred = pCurr;
                    pCurr = pSucc;
                } else {
                    break; // move to next iLevel
                }
            }
            if(NULL != preds)
                preds[iLevel] = pPred;
            if(NULL != succs)
                succs[iLevel] = pCurr;
        }

        if (pCurr->key == key) // bottom iLevel curr.key == key
        {
            return pCurr;
        }
        else
        {
            return NULL;

        }
    }
}



SkipList::SkipList() //:
    //_num_threads (_gNumThreads)
{
    max_level = MAX_LEVEL;

    head = new Node;
    head->top_level = max_level;
    head->key = 0;
    tail = new Node;
    tail->top_level = max_level;
    tail->key = INT_MAX;

    for (int iLevel = 0; iLevel < head->top_level; ++iLevel) {
        head->next[iLevel] = tail;
    }

    // Atomic allocation code - note that malloc has internal locks. It may or may not make a difference in
    // our implementation.

    //next_node_addr = (ulong_t) malloc(sizeof(Node)*(INITIAL_LIST_SIZE + TOTAL_NUM_OPS));
    //if (next_node_addr == 0) {
    //    cout << "Not enough memory! Exiting..." << endl;
    //    exit(1);
    //}
    next_node_addr = NULL;
}


void SkipList::set_max_level(ulong_t initsize)
{
    unsigned int i = 0;
    for (i = 1; i < MAX_LEVEL; i++) {
        if (initsize < (1 << i)) {
            max_level = i;
            break;
        }
    }
}


//enq ......................................................
bool SkipList::add(ulong_t x) {
    Node* preds[max_level + 1];
    Node* succs[max_level + 1];
    Node* pPred;
    Node* pSucc;
    Node* new_node = NULL;
    int topLevel = 0;

    while (true) {
        Node* found_node = find(x, preds, succs);
        // this disallows duplicates
        if(NULL != found_node) {
            return false;
        }

        // try to splice in new node in 0 going up
        pPred = preds[0];
        pSucc = succs[0];

        if (NULL == new_node) {
            topLevel = randomLevel();
            new_node = alloc_node();
            new_node->top_level = topLevel;
            new_node->key = x;
        }

        // prepare new node
        for (int iLevel = 0; iLevel < topLevel; ++iLevel) {
            new_node->next[iLevel] = PTR(succs[iLevel]);
        }

        if (!(__sync_bool_compare_and_swap(&pPred->next[0], PTR(pSucc), PTR(new_node)))) {
            continue; // retry from start
        }

        // splice in remaining levels going up
        for (int iLevel = 1; iLevel < topLevel; ++iLevel) {
            while (true) {
                pPred = preds[iLevel];
                pSucc = succs[iLevel];
                if (__sync_bool_compare_and_swap(&pPred->next[iLevel], PTR(pSucc), PTR(new_node))) {
                    break;
                }
                find(x, preds, succs); // find new preds and succs
            }
        }

        return true;
    }
}


bool SkipList::remove(ulong_t x) {
    Node* succs[max_level + 1];
    Node* preds[max_level + 1];
    Node* pSucc;
    Node* pNodeToRemove;
    ulong_t key = x;

    while (true) {
        bool found = find(key, preds, succs);
        if (!found) { //if found it's not marked
            return false;
        }
        else {
            //logically remove node
            pNodeToRemove = succs[0];

            for (int iLevel = pNodeToRemove->top_level - 1; iLevel > 0; --iLevel) {
                bool marked = false;
                pSucc = pNodeToRemove->next[iLevel];
                marked = MARKED(pSucc);
                pSucc = PTR(pSucc);
                while (!marked) { // until I succeed in marking
                    __sync_bool_compare_and_swap(&pNodeToRemove->next[iLevel], PTR(pSucc), MARK(pSucc));
                    pSucc = pNodeToRemove->next[iLevel];
                    marked = MARKED(pSucc);
                    pSucc = PTR(pSucc);
                }
            }

            // proceed to remove from bottom iLevel
            bool marked = false;
            pSucc = pNodeToRemove->next[0];
            marked = MARKED(pSucc);
            pSucc = PTR(pSucc);
            while (true) { // until someone succeeded in marking
                bool iMarkedIt = __sync_bool_compare_and_swap(&pNodeToRemove->next[0], PTR(pSucc), MARK(pSucc));
                pSucc = pNodeToRemove->next[0];
                marked = MARKED(pSucc);
                pSucc = PTR(pSucc);
                if (iMarkedIt) {
                    // run find to remove links of the logically removed node
                    findAndClean(key);
                    return true;
                }
                else {
                    if (marked) { // found and already logically removed by someone else
                        return false;
                    }
                    // if we get here, something completely different must have happened. we need to rerun.
                }
            }
        }
    }
}



bool SkipList::contains(ulong_t x) {
    Node* pPred = NULL;
    Node* pCurr = NULL;
    Node* pSucc = NULL;
    bool marked = 0;

    // search through host LF portion
    pPred = head;
    pCurr = head;
    for (int iLevel = (max_level - 1); iLevel >= 0; --iLevel) {
        while (true) {
            // pCurr is node we are interested in.
            // (pSucc (pCurr->next value at the current level)
            //  is accessed to check if pCurr is marked for removal.)
            pSucc = pCurr->next[iLevel]; 
            marked = MARKED(pSucc);

            while (marked) {
                // pCurr has been marked for removal.
                // iterate to find unmarked pCurr.
                pCurr = PTR(pCurr->next[iLevel]);
                pSucc = pCurr->next[iLevel];
                marked = MARKED(pSucc);
            }

            if (x > pCurr->key) {
                // move forward in same level
                pPred = pCurr;
                pCurr = PTR(pSucc);
            } else if (x == pCurr->key) {
                // key found
                return true;
            } else {
                // move to next level.
                // next level must begin search from pPred.
                pCurr = pPred;
                break;
            }
        } // while
    } // for all host levels

    return false;
#if 0
    Node* pCurr;
    Node* pSucc;

    bool marked = false;
    pCurr = head;
    for (int iLevel = max_level - 1; iLevel >= 0; --iLevel) {
        while (true) {
            pSucc = pCurr->next[iLevel];
            marked = MARKED(pSucc);
            pSucc = PTR(pSucc);
            while ( marked) {
                pCurr = PTR(pCurr->next[iLevel]);
                pSucc = pCurr->next[iLevel];
                marked = MARKED(pSucc);
                pSucc = PTR(pSucc);
            }

            if (x >= pSucc->key) { // move forward same iLevel
                pCurr = pSucc;
            } else {
                break; // move to next iLevel
            }
        }
    }
    return (x == pCurr->key);
#endif
}

/*
 * This function can be written to just return a new Node (uses system malloc, which may internally use locks)
 * or take advantage of a preallocated memory region to get a new Node in a lock-free manner. - jglister
 */

Node *SkipList::alloc_node() {
    //return (Node *) __sync_fetch_and_add(&next_node_addr, sizeof(Node));
    return new Node;
}

void SkipList::print()
{
    ulong_t level = max_level - 1;
    Node *curr = NULL;

    cout << "skiplist contents: " << endl;
    while (1) {
        cout << "at level " << level << endl;
        curr = head->next[level];
        while (curr->next[level] != NULL) {
            cout << curr->key << endl;
            curr = curr->next[level];
        }
        if (level == 0) { break; }
        level--;
    }
}


//------------------------------------------------------------------------------
// File    : LFSkipList.h
// Author  : Ms.Moran Tzafrir; email: morantza@gmail.com; phone: +972-505-779961
// Written : 13 April 2009
//
// Lock Free Skip List
//
// Copyright (C) 2009 Moran Tzafrir.
// You can use this file only by explicit written approval from Moran Tzafrir.
//------------------------------------------------------------------------------
//*/
#endif

