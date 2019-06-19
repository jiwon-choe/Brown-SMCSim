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
#ifdef HOST_DEBUG_ON
#include <vector>
#endif
#include "LFArrayQueue.h"

using namespace std;

typedef struct thread_data {
    int thread_id;
    ulong_t *rand_keys;
    int num_ops;
    MyQueue *q;
} thread_data_t;


char start_threads;


#ifdef HOST_DEBUG_ON
pthread_mutex_t debug_lock;
#endif


void * ops_routine(void *routine_args)
{
    int id = ((thread_data_t *)routine_args)->thread_id;
    MyQueue *q = ((thread_data_t *)routine_args)->q;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    int op_count = 0;
    ulong_t rand_key_value = 0; 
    ulong_t ret = 0;

#ifdef HOST_DEBUG_ON
    int i = 0;
    vector<string> operation_type;
    vector<int> operation_args;
    vector<int> operation_rslt;
#endif

    while (start_threads == 0) {
        // wait
    }

    while (op_count < num_ops) {
        if((op_count + id) % 2 == 0) {
            rand_key_value = rand_keys[op_count];
            q->enq(rand_key_value);
#ifdef HOST_DEBUG_ON
            operation_type.push_back("push");
            operation_args.push_back(rand_key_value);
#endif
        } else {
            ret = q->deq();
#ifdef HOST_DEBUG_ON
            operation_type.push_back("pop");
            operation_args.push_back(ret);
#endif
        }
        op_count++;
    } // while

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&debug_lock);
    for (i = 0; i < operation_type.size(); i++) {
        cout << "Print_from_Thread ID " << id << ": " << operation_type[i];
        cout << " " << operation_args[i] << endl;
    }
    pthread_mutex_unlock(&debug_lock);
#endif
    return NULL;
}



int main(int argc, char* argv[])
{
    PIMAPI *pim = new PIMAPI(); /* Instantiate the PIM API */
    MyQueue *queue = new MyQueue();
    thread_data_t thread_data[NUM_HOST_THREADS] = {0};
    pthread_t threads[NUM_HOST_THREADS] = {0};
    pthread_attr_t thread_attr[NUM_HOST_THREADS];
    cpu_set_t cpus;
    int x = 0;
    int i = 0;
    int j = 0;
    int ret = 0;
    int size = 0;
    ulong_t rand_key_value = 0;
    int num_threads = 0;
    srand (time(NULL));

    start_threads = 0;
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

    while (size < INITIAL_Q_SIZE) {
        rand_key_value = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        queue->enq(rand_key_value);
        size++;
    }
    cout << "(main.cpp): added initial elements to queue ... printing contents ..." << endl;
#ifdef HOST_DEBUG_ON
    queue->print();
#endif

    cout << "(main.cpp): testing with a total of " << TOTAL_NUM_OPS << " operations." << endl;
    cout << "(main.cpp): percentage of push operations is ";
    cout << PUSH_PERCENTAGE << endl;

#if 0
    start_threads = 0;

    for (i = 0; i < NUM_HOST_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].rand_keys = (ulong_t *)malloc(sizeof(ulong_t) * (TOTAL_NUM_OPS / NUM_HOST_THREADS));
        for (j = 0; j < (TOTAL_NUM_OPS / NUM_HOST_THREADS); j++) {
            thread_data[i].rand_keys[j] = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        }
        thread_data[i].q = queue;
        pthread_attr_init(&(thread_attr[i]));
        CPU_ZERO(&cpus);
        CPU_SET(i, &cpus);
        pthread_attr_setaffinity_np(&(thread_attr[i]), sizeof(cpu_set_t), &cpus);
    }

    for (i = 0; i < NUM_HOST_THREADS; i++) {
        ret = pthread_create(&(threads[i]), &(thread_attr[i]), ops_routine, (void *)(&(thread_data[i])));
        if (ret != 0) {
            cout << "failed to create thread " << i << endl;
            return ret;
        }
    }

    for (i = 0; i < 100000; i++) {
        // wait
    }

    ____TIME_STAMP(1);
    start_threads = 1;

    for (i = 0; i < NUM_HOST_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    ____TIME_STAMP(2);

#ifdef HOST_DEBUG_ON
    queue->print();
    pthread_mutex_destroy(&debug_lock);
#endif
#endif

    for (x = 1; x <= 4; x++) {
        num_threads = 2*x;
        start_threads = 0;

        for (i = 0; i < num_threads; i++) {
            thread_data[i].thread_id = i;
            thread_data[i].num_ops = (TOTAL_NUM_OPS / num_threads);
            thread_data[i].rand_keys = (ulong_t *)malloc(sizeof(ulong_t) * (thread_data[i].num_ops));
            for (j = 0; j < thread_data[i].num_ops; j++) {
                thread_data[i].rand_keys[j] = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
            }
            thread_data[i].q = queue;
            pthread_attr_init(&(thread_attr[i]));
            CPU_ZERO(&cpus);
            CPU_SET(i, &cpus);
            pthread_attr_setaffinity_np(&(thread_attr[i]), sizeof(cpu_set_t), &cpus);
        }

        for (i = 0; i < num_threads; i++) {
            ret = pthread_create(&(threads[i]), &(thread_attr[i]), ops_routine, (void *)(&(thread_data[i])));
            if (ret != 0) {
                cout << "failed to create thread " << i << endl;
                return ret;
            }
        }

        for (i = 0; i < 100000; i++) {
            // wait
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
        }
    } // for x

#ifdef HOST_DEBUG_ON
    queue->print();
    pthread_mutex_destroy(&debug_lock);
#endif

    APP_INFO("[---DONE---]")

    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

