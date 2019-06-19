#include "pim_api.hh"
#include "app_utils.hh"           // This must be included before anything else
#include "utils.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <stdlib.h>               // srand, rand
#include <time.h>                 // time
#include <unordered_map>
#include "utils.hh"
#include "FCPartSkipList.hh"
#include "FCSkipList.hh"
#ifdef HOST_DEBUG_ON
#include <vector>
#endif

using namespace std;

typedef struct thread_data {
    int thread_id;
    ulong_t *rand_keys;
    ulong_t *rand_ops;
    ulong_t num_ops;
    #ifdef FLATCOMBINE_PART
    PartSkipList *list;
    #else
    SkipList *list;
    #endif // FLATCOMBINE_PART
    SkipList::SlotInfo * volatile slot[NUM_PARTITIONS];
} thread_data_t;


char start_threads;

#ifdef HOST_DEBUG_ON
pthread_mutex_t debug_lock;
#endif

void * skiplist_ops_routine(void *routine_args)
{
    #ifdef FLATCOMBINE_PART
    PartSkipList *list = ((thread_data_t *)routine_args)->list;
    #else
    SkipList *list = ((thread_data_t *)routine_args)->list;
    #endif
    int id = ((thread_data_t *)routine_args)->thread_id;
    ulong_t num_ops = ((thread_data_t *)routine_args)->num_ops;
    ulong_t op_count = 0;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    ulong_t *rand_ops = ((thread_data_t *)routine_args)->rand_ops;
    ulong_t rand_key_value = 0;
    ulong_t rand_op_type = 0;
    int ret = 0;
    unsigned int i = 0;
    int op_timestamp = 0;
    SkipList::SlotInfo * volatile local_slot[NUM_PARTITIONS] = {0};
    int part_id = 0;
    int partition_size = (KEY_UPPER_BOUND - KEY_LOWER_BOUND + 1) / NUM_PARTITIONS;

    for (i = 0; i < NUM_PARTITIONS; i++) {
        //cout << "routine_args->slot[i] = " << ((thread_data_t *)routine_args)->slot[i] << endl;
        local_slot[i] = ((thread_data_t *)routine_args)->slot[i];
    }

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&debug_lock);
    pthread_mutex_unlock(&debug_lock);
    vector<string> operation_type;
    vector<int> operation_args;
    vector<int> operation_rslt;
    vector<int> operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {
        rand_key_value = rand_keys[op_count];
        rand_op_type = rand_ops[op_count];
        op_count++;
        //rand_key_value = (myrand(&rand_seed) % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        part_id = (rand_key_value - KEY_LOWER_BOUND) / partition_size;

        if (rand_op_type <= READ_ONLY_PERCENTAGE) {
            // contains
            ret = list->fc_operation(id, CONTAINS_OP, rand_key_value, part_id, local_slot[part_id]);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("contains");
#endif
        } else if (rand_op_type > (READ_ONLY_PERCENTAGE + (100-READ_ONLY_PERCENTAGE)/2)) {
            // add
            ret = list->fc_operation(id, ADD_OP, rand_key_value, part_id, local_slot[part_id]);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("add");
#endif
        } else {
            // remove
            ret = list->fc_operation(id, REMOVE_OP, rand_key_value, part_id, local_slot[part_id]);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("remove");
#endif
        }
#ifdef HOST_DEBUG_ON
        operation_args.push_back(rand_key_value);
        operation_rslt.push_back(ret);
        op_timestamp = local_slot[part_id]->op_timestamp;
        operation_timestamp.push_back(op_timestamp);
#endif
    }

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&debug_lock);
    for (i = 0; i < operation_type.size(); i++) {
        cout << "Print_from_Thread_ID " << id << ": " << operation_type[i];
        cout << " " << operation_args[i] << " " << operation_rslt[i] << " timestamp: ";
        cout << operation_timestamp[i] << endl;
    }
    pthread_mutex_unlock(&debug_lock);
#endif

    //delete local_slot;
    return NULL;
}



int main(int argc, char* argv[])
{
    PIMAPI *pim = new PIMAPI(); /* Instantiate the PIM API */
    #ifdef FLATCOMBINE_PART
    PartSkipList *skip_list = new PartSkipList(); // FC list
    #else
    SkipList *skip_list = new SkipList(); // FC list
    #endif
    thread_data_t thread_data[NUM_HOST_THREADS] = {0};
    pthread_t threads[NUM_HOST_THREADS] = {0};
    pthread_attr_t thread_attr[NUM_HOST_THREADS];
    cpu_set_t cpus;
    int size = 0;
    int i = 0;
    int j = 0;
    int x = 0;
    int num_threads = 0;
    ulong_t rand_key_value = 0;
    int rand_iterations = 0;
    std::unordered_map<ulong_t, ulong_t> randmap = {};
    int ret = 0;
    int part_id = 0;
    int partition_size = (KEY_UPPER_BOUND - KEY_LOWER_BOUND + 1) / NUM_PARTITIONS; 
    SkipList::SlotInfo * volatile main_slot[NUM_PARTITIONS];
    srand (time(NULL));

#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&debug_lock, NULL);
#endif

    for (i = 0; i < NUM_PARTITIONS; i++) {
        main_slot[i] = new SkipList::SlotInfo();
    }

    // offload dummy kernel
    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    pim->offload_kernel((char*)FILE_NAME);

    cout << "(main.cpp): lower bound for keys -- " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): upper bound for keys -- " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): initial size of skip list -- " << INITIAL_LIST_SIZE << endl;


    cout << "(main.cpp): adding initial nodes to skip list ... " << endl;

    skip_list->set_max_level(INITIAL_LIST_SIZE / NUM_PARTITIONS);
#if 1
    while (randmap.size() < INITIAL_LIST_SIZE) {
        rand_key_value = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        rand_iterations++;
        if (randmap.count(rand_key_value) == 0) {
            randmap[rand_key_value] = rand_key_value;
            if (randmap.size() % 10000 == 0) {
                cout << "reached " << randmap.size() << " items in initial randmap!" << endl;
            }
        }
    }

    cout << "took " << rand_iterations << " rand calls to get " << INITIAL_LIST_SIZE << "unique elements" << endl;

    for (std::pair<ulong_t, ulong_t> element : randmap) {
        part_id = (element.first - KEY_LOWER_BOUND) / partition_size;
        ret = skip_list->fc_operation(0, ADD_OP, element.first, part_id, main_slot[part_id]);
        if (ret == 1) {
            size++;
            if (size % 10000 == 0) {
                cout << size << " elements in initial skiplist!" << endl;
            }
        }
    }
#else // if 0
    while (size < INITIAL_LIST_SIZE) {
        rand_key_value = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        rand_iterations++;
        part_id = (rand_key_value - KEY_LOWER_BOUND) / partition_size;
        ret = skip_list->fc_operation(0, ADD_OP, rand_key_value, part_id, main_slot[part_id]);
        if (ret == 1) {
            size++;
            if (size % 10000 == 0)  {
                cout << size << " elements in initial skiplist!" << endl;
            }
        }
    }
#endif

    cout << "(main.cpp): added " << size << " elements to skip list ... printing contents ... " << endl;
#ifdef HOST_DEBUG_ON
    skip_list->print();
#endif

    //skip_list->reset_timestamp();
    cout << "(main.cpp): testing with a total of " << TOTAL_NUM_OPS << " operations." << endl;
    cout << "(main.cpp): percentage of read-only (contains) operations is ";
    cout << READ_ONLY_PERCENTAGE << endl;

    for (i = 0; i < NUM_HOST_THREADS; i++) {
        for (j = 0; j < NUM_PARTITIONS; j++) {
            thread_data[i].slot[j] = new SkipList::SlotInfo();
        }
    }

    for (x = 1; x <= 4; x++) {
        num_threads = 2*x;
        start_threads = 0;

        for (i = 0; i < num_threads; i++) {
            thread_data[i].thread_id = i;
            thread_data[i].list = skip_list;
            /*for (j = 0; j < NUM_PARTITIONS; j++) {
                thread_data[i].slot[j] = new SkipList::SlotInfo();
            }*/
            thread_data[i].num_ops = TOTAL_NUM_OPS / num_threads;
            thread_data[i].rand_keys = (ulong_t *)malloc(sizeof(ulong_t) * thread_data[i].num_ops);
            thread_data[i].rand_ops = (ulong_t *)malloc(sizeof(ulong_t) * thread_data[i].num_ops);
            for (j = 0; j < thread_data[i].num_ops; j++) {
                thread_data[i].rand_keys[j] = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
                thread_data[i].rand_ops[j] = rand() % 100; 
            }
        }

        for (i = 0; i < num_threads; i++) {
            pthread_attr_init(&(thread_attr[i]));
            CPU_ZERO(&cpus);
            CPU_SET(i, &cpus);
            pthread_attr_setaffinity_np(&(thread_attr[i]), sizeof(cpu_set_t), &cpus);
            ret = pthread_create(&(threads[i]), &(thread_attr[i]), skiplist_ops_routine, (void *)&(thread_data[i]));
            if (ret != 0) {
                cout << "(main.cc): Error in creating thread, threadID " << i << endl;
                return -1;
            }
        }

        for (j = 0; j < 1000; j++) {
            // wait for threads to initialize
        }
        ____TIME_STAMP(2*x-1);
        start_threads = 1;
        for (i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        ____TIME_STAMP(2*x);
        cout << "Done with " << num_threads << " threads!" << endl;
        for (i = 0; i < num_threads; i++) {
            free((void *)(thread_data[i].rand_keys));
            free((void *)(thread_data[i].rand_ops));
        }
    } // for x

#ifdef HOST_DEBUG_ON
    pthread_mutex_destroy(&debug_lock);
#endif

    APP_INFO("[---DONE---]")

    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

