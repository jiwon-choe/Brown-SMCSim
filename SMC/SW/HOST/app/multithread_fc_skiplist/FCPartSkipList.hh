#ifndef FCPARTLIST_H
#define FCPARTLIST_H

#include "app_utils.hh"  // ulong_t
#include "utils.hh"
#include <stdio.h>
#include <errno.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <time.h>
#include <vector>
#include "FCSkipList.hh"

/* Partitioned skiplist, in case any experiments wth it are needed.
 * Note that each partition has its own FC queue - there are now four
 * queues instead of one.
 */

class PartSkipList {
    public:
        PartSkipList();   // constructor
        PartSkipList(ulong_t minimum, ulong_t maximum);
        ~PartSkipList();  // destructor
        int fc_operation(int thread_id, int operation_type, ulong_t parameter, int part_id, SkipList::SlotInfo *slot);
        void set_max_level(ulong_t initsize);
        void print();
    private:
        int max_level;
        ulong_t min_key;
        ulong_t max_key;
        int num_partitions;
        ulong_t partition_size;
        SkipList * parts[NUM_PARTITIONS];
};


PartSkipList::PartSkipList():
    min_key(KEY_LOWER_BOUND),
    max_key(KEY_UPPER_BOUND),
    num_partitions(NUM_PARTITIONS)
{
    // Create partitions using the SkipList class
    cout << "num partitions: " << num_partitions << endl;
    ulong_t span = max_key - min_key + 1;
    partition_size = span / num_partitions;
    for(int i = 0; i < num_partitions; i++) {
#ifdef HOST_DEBUG_ON
        cout << "partition about to be created" << endl;
        cout << "new partition: range from " << partition_size*i + min_key << " to " << partition_size*(i+1) + min_key - 1 << endl;
#endif
        parts[i] = new SkipList(partition_size*i + min_key, partition_size*(i+1) + min_key - 1);
    }
}

// Alternate constructor using parameters rather than macros

PartSkipList::PartSkipList(ulong_t minimum, ulong_t maximum):
    min_key(minimum),
    max_key(maximum),
    num_partitions(NUM_PARTITIONS)
{
    // Create partitions usng the SkipList class
    ulong_t span = max_key - min_key + 1;
    partition_size = span / num_partitions;
    for (int i = 0; i < num_partitions; i++) {
        parts[i] = new SkipList(partition_size*i + min_key, partition_size*(i+1) + min_key - 1);
    }
}


PartSkipList::~PartSkipList()
{

}

void PartSkipList::set_max_level(ulong_t initsize)
{
    int i = 0;
    for (i = 0; i < num_partitions; i++) {
        parts[i]->set_max_level(initsize);
    }
}

int PartSkipList::fc_operation(int thread_id, int operation_type, ulong_t parameter, int part_id, SkipList::SlotInfo *slot)
{
    return parts[part_id]->fc_operation(thread_id, operation_type, parameter, slot);
}


void PartSkipList::print()
{
    for (int i = 0; i < num_partitions; i++) {
        parts[i]->print();
    }
}


#endif // ifndef FCLINKEDLIST_H
