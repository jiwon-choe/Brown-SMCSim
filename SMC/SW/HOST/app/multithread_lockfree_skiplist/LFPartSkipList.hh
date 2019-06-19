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
#include "LFSkipList.hh"

// Partitioned lock free skiplist, in case any experiments using it are necessary.

class PartSkipList {
    public:
        PartSkipList();   // constructor
        PartSkipList(ulong_t minimum, ulong_t maximum);
        ~PartSkipList();  // destructor
        bool add(ulong_t x);
        bool contains(ulong_t x);
        bool remove(ulong_t x);
        void print();
    private:
        int max_level;
        ulong_t min_key;
        ulong_t max_key;
        int num_partitions;
        ulong_t partition_size;
        //std::vector<SkipList *> parts;
        SkipList * parts[NUM_PARTITIONS];
};


PartSkipList::PartSkipList():
    min_key(KEY_LOWER_BOUND),
    max_key(KEY_UPPER_BOUND),
    num_partitions(NUM_PARTITIONS)
{
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

PartSkipList::PartSkipList(ulong_t minimum, ulong_t maximum):
    min_key(minimum),
    max_key(maximum),
    num_partitions(NUM_PARTITIONS)
{
    ulong_t span = max_key - min_key + 1;
    partition_size = span / num_partitions;
    for (int i = 0; i < num_partitions; i++) {
        parts[i] = new SkipList(partition_size*i + min_key, partition_size*(i+1) + min_key - 1);
    }
}


PartSkipList::~PartSkipList()
{

}

// In all cases, just compute which partition the operation should be performed on
// and just do it

bool PartSkipList::add(ulong_t x) {
    int part_id = (x - KEY_LOWER_BOUND) / partition_size;
    return parts[part_id]->add(x);
}
bool PartSkipList::contains(ulong_t x) {
    int part_id = (x - KEY_LOWER_BOUND) / partition_size;
    return parts[part_id]->contains(x);
}
bool PartSkipList::remove(ulong_t x) {
    int part_id = (x - KEY_LOWER_BOUND) / partition_size;
    return parts[part_id]->remove(x);
}


void PartSkipList::print()
{
    for (int i = 0; i < num_partitions; i++) {
        parts[i]->print();
    }
}


#endif // ifndef FCLINKEDLIST_H
