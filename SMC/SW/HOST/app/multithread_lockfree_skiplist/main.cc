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
#ifdef LOCKFREE
#include "LFSkipList.hh"
#endif
#ifdef HOST_DEBUG_ON
#include <vector>
#endif

using namespace std;

typedef struct thread_data {
    int thread_id;
    ulong_t *rand_keys;
    ulong_t *rand_ops;
    ulong_t num_ops;
    SkipList *list;
} thread_data_t;

char start_threads;

#ifdef HOST_DEBUG_ON
pthread_mutex_t dbglock;
#endif




#define LIST_SIZE (KEY_UPPER_BOUND - KEY_LOWER_BOUND + 1)

void * skiplist_ops_routine(void *routine_args)
{
    SkipList *list = ((thread_data_t *)routine_args)->list;
    int thread_id = ((thread_data_t *)routine_args)->thread_id;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    ulong_t *rand_ops = ((thread_data_t *)routine_args)->rand_ops;
    ulong_t key_value = 0;
    int op_count = 0;
    ulong_t num_ops = ((thread_data_t *)routine_args)->num_ops;
    int rand_op_type = 0;
    int ret = 0;

#ifdef HOST_DEBUG_ON
	vector<string> operation_type;
	vector<int>    operation_args;
	vector<int>    operation_rslt;
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
			operation_args.push_back(key_value);
			operation_rslt.push_back(ret);
#endif
        } else if (rand_op_type % 2 == 0) {
            // add
            ret = list->add(key_value);
#ifdef HOST_DEBUG_ON
			operation_type.push_back("add");
			operation_args.push_back(key_value);
			operation_rslt.push_back(ret);
#endif
        } else {
            // remove
            ret = list->remove(key_value);
#ifdef HOST_DEBUG_ON
			operation_type.push_back("remove");
			operation_args.push_back(key_value);
			operation_rslt.push_back(ret);
#endif
        }
    }

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&dbglock);
	for (unsigned int i = 0; i < operation_type.size(); i++) {
		cout << "Thread ID " << thread_id << ": " << operation_type[i];
		cout << "(" << operation_args[i] << ") " << operation_rslt[i] << endl;
	}
    pthread_mutex_unlock(&dbglock);
#endif
    return NULL;
}



int main(int argc, char* argv[])
{
    PIMAPI *pim = new PIMAPI(); /* Instantiate the PIM API */
    SkipList* int_list = new SkipList(); // lock-based linked list
    thread_data_t thread_data[NUM_HOST_THREADS] = {0};
    pthread_t threads[NUM_HOST_THREADS] = {0};
    pthread_attr_t thread_attr[NUM_HOST_THREADS];
    cpu_set_t cpus;
    int size = 0;
    int x = 0;
    unsigned int i = 0;
    int j = 0; 
    int key_value = 0;
    int ret = 0;
    int num_threads = 0;
    srand (time(NULL));
#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&dbglock, NULL);
#endif

    // offload dummy kernel
    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    pim->offload_kernel((char*)FILE_NAME);

    int_list->set_max_level(INITIAL_LIST_SIZE);

    int_list->print();

    cout << "(main.cpp): lower bound for keys -- " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): upper bound for keys -- " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): initial size of linked list -- " << INITIAL_LIST_SIZE << endl;

    cout << "(main.cpp): adding initial nodes to linked list ... " << endl;

#ifdef HOST_DEBUG_ON
    vector<ulong_t> initial_keys;
#endif
    // populate linked list with initial values
    while (size < INITIAL_LIST_SIZE){
        key_value = rand()%LIST_SIZE + KEY_LOWER_BOUND;// generate a random value in the range
        //cout << "(main.cpp): add(" << key_value << ")" << endl;
        ret = int_list->add(key_value);               // add(key_value) to my list
        if (ret) {
            size++;
            if (size % 10000 == 0) {
                cout << "added " << size << " elements to skiplist" << endl;
            }
#ifdef HOST_DEBUG_ON
			initial_keys.push_back(key_value);
            cout << "(main.cpp): current list size = " << size << endl;
#endif
        }
    }
    cout << "(main.cpp): added " << size << " elements to skiplist ... printing contents ... " << endl;
#ifdef HOST_DEBUG_ON
    cout << "Set in order: " << endl;
    int_list->print();
    cout << "Keys: " << endl;
	for (i=0; i<initial_keys.size(); i++)
		cout << initial_keys[i] << " ";
	cout << endl;
#endif

    cout << "(main.cpp): testing with a total of " << TOTAL_NUM_OPS << " operations." << endl;
    cout << "(main.cpp): percentage of read-only (contains) operations is ";
    cout << READ_ONLY_PERCENTAGE << endl;

    for (x = 1; x <= 4; x++) {
        num_threads = 2*x;
        start_threads = 0;

        for (i = 0; i < num_threads; i++) {
            thread_data[i].thread_id = i;
            thread_data[i].list = int_list;
            thread_data[i].num_ops = TOTAL_NUM_OPS / num_threads;
            thread_data[i].rand_keys = (ulong_t *)malloc(thread_data[i].num_ops * sizeof(ulong_t));
            thread_data[i].rand_ops = (ulong_t *)malloc(thread_data[i].num_ops * sizeof(ulong_t));
            
            for (j = 0; j < thread_data[i].num_ops; j++) {
                thread_data[i].rand_keys[j] = rand() % KEY_UPPER_BOUND + KEY_LOWER_BOUND;
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

        for (i = 0; i < 10000; i++) {
            //wait
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
    pthread_mutex_destroy(&dbglock);
#endif

    APP_INFO("[---DONE---]")

    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}














