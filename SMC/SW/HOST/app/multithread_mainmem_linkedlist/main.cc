#include "pim_api.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <stdlib.h> // srand, rand
#include <time.h>
#include <stdint.h> // UINT32_MAX
#include "app_utils.hh"     // This must be included before anything else
using namespace std;
#include "utils.hh"
#include <unordered_map>
#ifdef HOST_FC_SORT
#include "FCLinkedList.h"
#endif
#ifdef HOST_LAZY_LOCK
#include "LazyLockList.h"
#endif


typedef struct thread_data {
    int thread_id;
#ifdef PIM_FC_SORT
    PIMAPI *pimapi;
#endif
#ifdef HOST_FC_SORT
    LinkedList *list;
    LinkedList::SlotInfo *volatile slot;
#endif
#ifdef HOST_LAZY_LOCK
    LinkedList *list;
#endif
    ulong_t *rand_keys;
    ulong_t *rand_ops;
    int num_ops;
} thread_data_t;


volatile char start_threads;

#ifdef HOST_DEBUG_ON
pthread_mutex_t mylock;
#endif


ulong_t random_level() {
    ulong_t x = rand() % (1 << MAX_LEVEL);
    ulong_t level = 2;
    if ((x & 0x80000001) != 0) {
        return 1;
    }
    while (((x >>= 1) & 1) != 0) {
        level++;
    }
    if (level > (MAX_LEVEL - 1)) {
        return MAX_LEVEL - 1;
    }
    return level;
}


/************************************************/

#ifdef PIM_FC_SORT

#define LINKED_LIST_OPERATION(_op, _key) { \
        request_slot.parameter = _key; \
        pimapi->write_parameter(request_slot.parameter, thread_id); \
        SET_OPERATION(request_slot.info_bits, _op); \
        pimapi->write_info_bits(request_slot.info_bits, thread_id); \
        SET_REQUEST(request_slot.info_bits); \
        pimapi->write_info_bits(request_slot.info_bits, thread_id); \
        while (CHECK_VALID_REQUEST(request_slot.info_bits)) { \
            request_slot.info_bits = pimapi->read_info_bits(thread_id); \
        } \
        pim_res = GET_RETVAL(request_slot.info_bits); \
    }


void * linkedlist_ops_routine(void *routine_args) 
{
    int thread_id = ((thread_data_t *)routine_args)->thread_id;
    PIMAPI *pimapi = ((thread_data_t *)routine_args)->pimapi;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    ulong_t *rand_ops = ((thread_data_t *)routine_args)->rand_ops;
    request_slot_t request_slot;
    request_slot.info_bits = 0;
    request_slot.timestamp = 0;
    request_slot.parameter = 0;
    int key_value = 0;
    int op_count = 0;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    ulong_t rand_op_type = 0;
    int pim_res = 0;
#ifdef HOST_DEBUG_ON
	vector<string> operation_type;
	vector<int>    operation_args;
	vector<int>    operation_rslt;
    vector<int>    operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {
        key_value = rand_keys[op_count];
        rand_op_type = rand_ops[op_count];
        op_count++;

        if (rand_op_type < READ_ONLY_PERCENTAGE) {
            // contains
            LINKED_LIST_OPERATION(CONTAINS_OP, key_value);
#ifdef HOST_DEBUG_ON
			operation_type.push_back("contains");
			operation_args.push_back(key_value);
			operation_rslt.push_back(pim_res);
#endif
        } else if (rand_op_type % 2 == 0) {
            // add
            LINKED_LIST_OPERATION(ADD_OP, key_value);
#ifdef HOST_DEBUG_ON
			operation_type.push_back("add");
			operation_args.push_back(key_value);
			operation_rslt.push_back(pim_res);
#endif
        } else {
            // remove
            LINKED_LIST_OPERATION(REMOVE_OP, key_value);
#ifdef HOST_DEBUG_ON
			operation_type.push_back("remove");
			operation_args.push_back(key_value);
			operation_rslt.push_back(pim_res);
#endif
        }
#ifdef HOST_DEBUG_ON
        request_slot.timestamp = pimapi->read_timestamp(thread_id);
        operation_timestamp.push_back(request_slot.timestamp);
#endif
    }

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&mylock);
	for (unsigned int i = 0; i < operation_type.size(); i++) {
		cout << "Print_from_Thread_ID " << thread_id << ": " << operation_type[i] << " ";
		cout << operation_args[i] << " " << operation_rslt[i] << " timestamp: ";
		cout << operation_timestamp[i] << endl;
	}
    pthread_mutex_unlock(&mylock);
#endif
    return NULL;
}

#endif // PIM_FC_SORT

/************************************************/

/************************************************/
#ifdef HOST_FC_SORT
void * linkedlist_ops_routine(void *routine_args)
{
    int thread_id = ((thread_data_t *)routine_args)->thread_id;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    ulong_t *rand_ops = ((thread_data_t *)routine_args)->rand_ops;
    LinkedList *list = ((thread_data_t *)routine_args)->list;
    LinkedList::SlotInfo * volatile local_slot = ((thread_data_t *)routine_args)->slot; 
    int key_value = 0;
    int op_count = 0;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    ulong_t rand_op_type = 0;
    int ret = 0;
#ifdef HOST_DEBUG_ON
    int op_timestamp = 0;
    int i = 0;
	vector<string> operation_type;
	vector<int>    operation_args;
	vector<int>    operation_rslt;
    vector<int>    operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {
        key_value = rand_keys[op_count];
        rand_op_type = rand_ops[op_count];
        op_count++;

        if (rand_op_type < READ_ONLY_PERCENTAGE) {
            // contains
            ret = list->fc_operation(thread_id, CONTAINS_OP, key_value, local_slot);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("contains");
#endif
        } else if (rand_op_type % 2 == 0) {
            // add
            ret = list->fc_operation(thread_id, ADD_OP, key_value, local_slot);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("add");
#endif
        } else {
            // remove
            ret = list->fc_operation(thread_id, REMOVE_OP, key_value, local_slot);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("remove");
#endif
        }
#ifdef HOST_DEBUG_ON
        operation_args.push_back(key_value);
        operation_rslt.push_back(ret);
        op_timestamp = local_slot->op_timestamp;
        operation_timestamp.push_back(op_timestamp);
#endif
    }

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&mylock);
    for (i = 0; i < operation_type.size(); i++) {
        cout << "Print_from_Thread ID " << thread_id << ": " << operation_type[i];
        cout << " " << operation_args[i] << " " << operation_rslt[i] << " timestamp: ";
        cout << operation_timestamp[i] << endl;
    }
    pthread_mutex_unlock(&mylock);
#endif

    return NULL;
}
#endif
/************************************************/

/************************************************/
#ifdef HOST_LAZY_LOCK
void * linkedlist_ops_routine(void *routine_args)
{
    int thread_id = ((thread_data_t *)routine_args)->thread_id;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    ulong_t *rand_ops = ((thread_data_t *)routine_args)->rand_ops;
    LinkedList *list = ((thread_data_t *)routine_args)->list;
    int key_value = 0;
    int op_count = 0;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    ulong_t rand_op_type = 0;
    int ret = 0;
#ifdef HOST_DEBUG_ON
    int i = 0;
	vector<string> operation_type;
	vector<int>    operation_args;
	vector<int>    operation_rslt;
    vector<int>    operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {
        key_value = rand_keys[op_count];
        rand_op_type = rand_ops[op_count];
        op_count++;

        if (rand_op_type < READ_ONLY_PERCENTAGE) {
            // contains
            ret = list->contains(key_value);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("contains");
#endif
        } else if (rand_op_type % 2 == 0) {
            // add
            ret = list->add(key_value);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("add");
#endif
        } else {
            // remove
            ret = list->remove(key_value);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("remove");
#endif
        }
#ifdef HOST_DEBUG_ON
        operation_args.push_back(key_value);
        operation_rslt.push_back(ret);
        operation_timestamp.push_back(op_count);
#endif
    }

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&mylock);
    for (i = 0; i < operation_type.size(); i++) {
        cout << "Print_from_Thread ID " << thread_id << ": " << operation_type[i];
        cout << " " << operation_args[i] << " " << operation_rslt[i] << " timestamp: ";
        cout << operation_timestamp[i] << endl;
    }
    pthread_mutex_unlock(&mylock);
#endif

    return NULL;
}
#endif
/************************************************/

/************************************************/
// Main
int main(int argc, char *argv[])
{
    init_region();
#ifdef PIM_FC_SORT
    void *aligned_region_ptr = 0;
    void *aligned_region_end = 0;
    PIMTask *task = new PIMTask("linkedlist-task");
#endif
#ifdef HOST_FC_SORT
    LinkedList *linked_list = NULL;
    LinkedList::SlotInfo *slot[NUM_HOST_THREADS] = {0};
#endif
#ifdef HOST_LAZY_LOCK
    LinkedList *linked_list = NULL;
#endif
    srand(time(NULL));
    PIMAPI *pim = new PIMAPI(); /* Instatiate the PIM API */ 
    int size = 0;
    skiplist_node_t *list_head = (skiplist_node_t *)allocate_region(sizeof(skiplist_node_t));
    skiplist_node_t *list_tail = (skiplist_node_t *)allocate_region(sizeof(skiplist_node_t));
    int ret = 0;
    int i = 0;
    int j = 0;
    ulong_t key = 0;
    ulong_t level = 0;
    pthread_t threads[NUM_HOST_THREADS];
    thread_data_t thread_data[NUM_HOST_THREADS];
    pthread_attr_t thread_attr[NUM_HOST_THREADS];
    cpu_set_t cpus;
    std::unordered_map<ulong_t, ulong_t> randmap = {};
    int rand_iterations = 0;
    int x = 0;
    int num_threads = 0;

#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&mylock, NULL);
#endif


    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    pim->offload_kernel((char*)FILE_NAME);

    cout << "(main.cpp): prepare initial linked list ... " << endl;
    list_head->key = 0;
    list_head->top_level = MAX_LEVEL;
    list_tail->key = UINT32_MAX;
    list_tail->top_level = MAX_LEVEL;
    for (i = 0; i < MAX_LEVEL; i++) {
        list_head->next[i] = list_tail;
        list_tail->next[i] = (skiplist_node_t *)0;
    }
    cout << "(main.cpp): key lower bound: " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): key upper bound: " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): initial list size: " << INITIAL_LIST_SIZE << endl;


    while (randmap.size() < INITIAL_LIST_SIZE) {
        key = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        rand_iterations++;
        if (randmap.count(key) == 0) {
            randmap[key] = key;
            if ((randmap.size() % 1000) == 0) {
                cout << "reached " << randmap.size() << " items in initial randmap!!" << endl;
            }
        }
    }
    cout << "took " << rand_iterations << " rand calls to get " << INITIAL_LIST_SIZE << " unique elements" << endl;
    for (std::pair<ulong_t, ulong_t> element : randmap) {
        level = random_level();
        ret = list_add(list_head, element.first, level);
        if (ret == 1) {
            size++;
            if ((size % 1000) == 0) {
                cout << "reached " << size << " items in initial list!!" << endl;
            }
        }
    }
    cout << "added total " << size << " elements to linked list!!" << endl;

#ifdef HOST_DEBUG_ON
    cout << "(main.cpp): printing initial list ... " << endl;
    list_print((node_t *)list_head);
#endif

#ifdef PIM_FC_SORT
    /* PIM Scalar Registers
    SREG[0]: list_head
    SREG[1]: PIM operation type (0: linked list ops, 1: quit)
    SREG[2]: operation timestamp for debugging
    SREG[3]: region_aligned ptr from app_utils.hh (needed in PIM for allocating new region)
    SREG[4]: region_end ptr from app_utils.hh (needed in PIM for allocating new region)
    SREG[5]: ---
    SREG[6]: ---
    SREG[7]: ---
    */

    task->addData(list_head, REQUIRED_MEM_SIZE, 0); // offload entire allocated region
    cout << "(main.cpp): Offloading the task ... " << endl;
    pim->offload_task(task);

    cout << "(main.cpp): Start computation ... " << endl;
    pim->write_sreg(1,0); // linked list ops
    pim->write_sreg(2,0); // operation timestamp 
    aligned_region_ptr = get_region_aligned_ptr();
    aligned_region_end = get_region_end_ptr();
    pim->write_sreg(3,(ulong_t)aligned_region_ptr);
    pim->write_sreg(4,(ulong_t)aligned_region_end);
    pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
#endif // PIM_FC_SORT

#ifdef HOST_FC_SORT
    for (i = 0; i < NUM_HOST_THREADS; i++) {
        slot[i] = new LinkedList::SlotInfo();
    }
#endif

    for (x = 1; x <= 4; x++) {
        // timestamp2 -- 2 threads, timestamp4 -- 4 threads
        // timestamp6 -- 6 threads, timestamp8 -- 8 threads
        num_threads = 2*x;
        start_threads = 0;
#if defined(HOST_FC_SORT) || defined(HOST_LAZY_LOCK)
        linked_list = new LinkedList((node_t *)list_head, (node_t *)list_tail);
#endif

        for (i = 0; i < num_threads; i++) {
            thread_data[i].rand_keys = (ulong_t *)malloc(sizeof(ulong_t) * (TOTAL_NUM_OPS / num_threads));
            thread_data[i].rand_ops = (ulong_t *)malloc(sizeof(ulong_t) * (TOTAL_NUM_OPS / num_threads));
            for (j = 0; j < (TOTAL_NUM_OPS / num_threads); j++) {
                thread_data[i].rand_keys[j] = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
                thread_data[i].rand_ops[j] = rand() % 100;
            }
        }

        for (i = 0; i < num_threads; i++) {
            thread_data[i].thread_id = i;
#ifdef PIM_FC_SORT
            thread_data[i].pimapi = pim;
#endif
#ifdef HOST_FC_SORT
            thread_data[i].list = linked_list;
            thread_data[i].slot = slot[i];
#endif
#ifdef HOST_LAZY_LOCK
            thread_data[i].list = linked_list;
#endif
            thread_data[i].num_ops = (TOTAL_NUM_OPS / num_threads);
            pthread_attr_init(&(thread_attr[i]));
            CPU_ZERO(&cpus);
            CPU_SET(i, &cpus);
            pthread_attr_setaffinity_np(&(thread_attr[i]), sizeof(cpu_set_t), &cpus);
            ret = pthread_create(&(threads[i]), &(thread_attr[i]), linkedlist_ops_routine, (void *)(&(thread_data[i])));
            if (ret != 0) {
                cout << "(main.cc): Error in creating list routine thread, threadID " << i << ": failed with error number " << ret << endl;
                return -1;
            }
        }
        ____TIME_STAMP((2*x-1));        
        start_threads = 1;

        // join list operation threads
        for (i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        ____TIME_STAMP((2*x));
        cout << "(main.cc): Done with " << num_threads << " threads!" << endl;
#if defined(HOST_FC_SORT) || defined(HOST_LAZY_LOCK)
        linked_list->~LinkedList();
#endif
        
        for (i = 0; i < num_threads; i++) {
            free((void *)(thread_data[i].rand_keys));
            free((void *)(thread_data[i].rand_ops));
#ifdef HOST_FC_SORT
            (thread_data[i].slot)->is_request_valid = 0;
            (thread_data[i].slot)->operation = 0;
            (thread_data[i].slot)->parameter = 0;
            (thread_data[i].slot)->ret_val = 0;
            (thread_data[i].slot)->op_timestamp = 0;
            (thread_data[i].slot)->is_slot_linked = 0;
            (thread_data[i].slot)->next_slot = NULL;
#endif
        }
    }

    APP_INFO("[---DONE---]");

#ifdef PIM_FC_SORT
    pim->write_sreg(1,1); // close PIM
#endif


    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

