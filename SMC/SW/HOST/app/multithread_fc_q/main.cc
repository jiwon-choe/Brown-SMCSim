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
#include "utils.hh"
#ifdef DOUBLE_FC
#include "DoubleFCArrayQueue.hh"
#else
#include "FCArrayQueue.hh"
#endif
#ifdef HOST_DEBUG_ON
#include <vector>
#endif

using namespace std;

typedef struct thread_data {
    int thread_id;
    ulong_t *rand_keys;
    int num_ops;
    Queue *q;
#ifdef DOUBLE_FC
    Queue::SlotInfo * volatile enqslot;
    Queue::SlotInfo * volatile deqslot;
#else
    Queue::SlotInfo * volatile slot;
#endif
} thread_data_t;


char start_threads;

#ifdef HOST_DEBUG_ON
pthread_mutex_t debug_lock;
#endif

void * queue_ops_routine(void *routine_args)
{
    Queue *q = ((thread_data_t *)routine_args)->q;
    int id = ((thread_data_t *)routine_args)->thread_id;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    int op_count = 0;
    ulong_t rand_key_value = 0;
    int ret = 0;
    unsigned int i = 0;
    int op_timestamp = 0;
#ifdef DOUBLE_FC
    Queue::SlotInfo * volatile local_enqslot = ((thread_data_t *)routine_args)->enqslot; 
    Queue::SlotInfo * volatile local_deqslot = ((thread_data_t *)routine_args)->deqslot; 
#else
    Queue::SlotInfo * volatile local_slot = ((thread_data_t *)routine_args)->slot; 
#endif

#ifdef HOST_DEBUG_ON
    vector<string> operation_type;
    vector<int> operation_args;
    vector<int> operation_rslt;
    vector<int> operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {

        if ((op_count + id) % 2 == 0) {
            // enq
            rand_key_value = rand_keys[op_count];
#ifdef DOUBLE_FC
            ret = q->fc_push_operation(id, rand_key_value, local_enqslot);
#else
            ret = q->fc_operation(id, PUSH_OP, rand_key_value, local_slot);
#endif
#ifdef HOST_DEBUG_ON
            op_timestamp = local_enqslot->op_timestamp;
            operation_type.push_back("push");
#endif
        } else {
            // deq
#ifdef DOUBLE_FC
            ret = q->fc_pop_operation(id, local_deqslot);
#else
            ret = q->fc_operation(id, POP_OP, rand_key_value, local_slot);
#endif
#ifdef HOST_DEBUG_ON
            op_timestamp = local_deqslot->op_timestamp;
            operation_type.push_back("pop");
#endif
        }
#ifdef HOST_DEBUG_ON
        operation_args.push_back(rand_key_value);
        operation_rslt.push_back(ret);
#ifndef DOUBLE_FC
        op_timestamp = local_slot->op_timestamp;
#endif
        operation_timestamp.push_back(op_timestamp);
#endif
        op_count++;
    } // while 

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&debug_lock);
    for (i = 0; i < operation_type.size(); i++) {
        cout << "Print_from_Thread ID " << id << ": " << operation_type[i];
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
    Queue *queue = new Queue();
    thread_data_t thread_data[NUM_HOST_THREADS] = {0};
    pthread_t threads[NUM_HOST_THREADS] = {0};
    pthread_attr_t thread_attr[NUM_HOST_THREADS];
    cpu_set_t cpus;
    int size = 0;
    unsigned int i = 0;
    int j = 0;
    int x = 0;
    int num_threads = 0;
    int rand_key_value = 0;
    int ret = 0;
    Queue::SlotInfo * volatile main_slot = new Queue::SlotInfo();
    srand (time(NULL));

#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&debug_lock, NULL);
#endif

    // offload dummy kernel
    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    pim->offload_kernel((char*)FILE_NAME);

    cout << "(main.cpp): lower bound for keys -- " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): upper bound for keys -- " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): initial size of queue -- " << INITIAL_Q_SIZE << endl;

    // TODO: contaminate memory manager (with random mallocs and frees)

    cout << "(main.cpp): adding initial nodes to queue ... " << endl;

    // populate linked list with initial values
    while (size < INITIAL_Q_SIZE){
        rand_key_value = rand()%KEY_UPPER_BOUND + KEY_LOWER_BOUND;// generate a random value in the range
#ifdef DOUBLE_FC
        ret = queue->fc_push_operation(0, rand_key_value, main_slot);
#else
        ret = queue->fc_operation(0, PUSH_OP, rand_key_value, main_slot);
#endif
        if (ret > 0) {
            size++;
        }
    }
    cout << "(main.cpp): added " << size << " elements to queue ... printing contents ... " << endl;
#ifdef HOST_DEBUG_ON
    queue->print();
#endif
    cout << "(main.cpp): testing with a total of " << TOTAL_NUM_OPS << " operations." << endl;
    cout << "(main.cpp): percentage of push operations is ";
    cout << PUSH_PERCENTAGE << endl;

    // initialize slots first
    for (i = 0; i < NUM_HOST_THREADS; i++) {
#ifdef DOUBLE_FC
        thread_data[i].enqslot = new Queue::SlotInfo();
        thread_data[i].deqslot = new Queue::SlotInfo();
#else
        thread_data[i].slot = new Queue::SlotInfo();
#endif
    }

    for (x = 1; x <= 4; x++) {
        num_threads = 2*x;
        start_threads = 0;

        for (i = 0; i < num_threads; i++) {
            thread_data[i].thread_id = i;
            thread_data[i].num_ops = (TOTAL_NUM_OPS / num_threads);
            thread_data[i].q = queue;
            thread_data[i].rand_keys = (ulong_t *)malloc(sizeof(ulong_t) * thread_data[i].num_ops);
            for (j = 0; j < thread_data[i].num_ops; j++) {
                thread_data[i].rand_keys[j] = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
            }

            pthread_attr_init(&(thread_attr[i]));
            CPU_ZERO(&cpus);
            CPU_SET(i, &cpus);
            pthread_attr_setaffinity_np(&(thread_attr[i]), sizeof(cpu_set_t), &cpus);
        }

        for (i = 0; i < num_threads; i++) {
            ret = pthread_create(&(threads[i]), &(thread_attr[i]), queue_ops_routine, (void *)&(thread_data[i]));
            if (ret != 0) {
                cout << "(main.cc): Error in creating thread, threadID " << i << endl;
                return -1;
            }
        }

        for (i = 0; i < 10000; i++) {
            // wait
        }
        ____TIME_STAMP(2*x-1);
        start_threads = 1;
        // join list operation threads
        for (i = 0; i < NUM_HOST_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }
        ____TIME_STAMP(2*x);
        cout << "Done with " << num_threads << " threads!" << endl;

        for (i = 0; i < num_threads; i++) {
            free((void *)(thread_data[i].rand_keys));
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

