#include "pim_api.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include "app_utils.hh"     // This must be included before anything else
#include <stdlib.h>         // srand(), rand()
#include <time.h>           // time()
#include <unordered_map>
#include "utils.hh"
#ifdef HOST_DEBUG_ON
#include <vector>
#endif
using namespace std;

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

#ifdef HOST_DEBUG_ON
pthread_mutex_t mylock;
#endif

typedef struct thread_data {
    int thread_id;
    PIMAPI *pimapi;
    ulong_t *rand_keys;
    ulong_t *rand_ops;
    int num_ops;
} thread_data_t;

char start_threads;


ulong_t rand_level() {
    ulong_t x = rand() % (1 << 14);
    ulong_t level = 2;
    if ((x & 0x80000001) != 0) {
        return 1;
    }
    while (((x >>= 1) & 1) != 0) {
        level++;
    }
    if (level > 13) {
        return 13;
    }
    return level;
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
    int rand_op_type = 0;
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

        if (rand_op_type <= READ_ONLY_PERCENTAGE) {
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

/************************************************/
// Main
int main(int argc, char *argv[])
{
    PIMAPI *pim = new PIMAPI(); /* Instatiate the PIM API */ 
    request_slot_t request_slot;
    request_slot.info_bits = 0;
    request_slot.timestamp = 0;
    request_slot.parameter = 0;
    pthread_t threads[NUM_LIST_THREADS] = {0};
    thread_data_t thread_data[NUM_LIST_THREADS] = {0};
    pthread_attr_t thread_attr[NUM_LIST_THREADS];
    cpu_set_t cpus;
    std::unordered_map<ulong_t, ulong_t> randmap = {};
    int rand_iterations = 0;
    int i = 0;
    int j = 0;
    int x = 0;
    int num_threads = 0;
    int thread_create_ret = 0;
    ulong_t pim_res = 0;
    int size = 0;
    ulong_t key_value = 0;           // this variable holds the key for a node
    srand(time(NULL));          // seed the RNG with time

#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&mylock, NULL);
#endif

    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    pim->offload_kernel((char*)FILE_NAME);

    /* PIM scalar registers
    SREG[0]: PIM operation
                0 -- init, 
                1 -- linked list operations 
                2 -- print list
    SREG[1]: maximum linked list key allowed on PIM
    */
    /* linked list test code */
    // checks for valid inputs

    cout << "(main.cpp): Prepare PIM kernel for linked list operations ... " << endl;
    pim->write_sreg(1,0); // reset completion flag before starting computation
    pim->write_sreg(0,0);
    // this should be the only place where start_computation is called
    pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
    // when setup on kernel completes, SREG[0] is set to 1 by the kernel (linked list ops)
    while (pim->read_sreg(1) == 0) {
        // wait
    }
    cout << "maximum # of items allowed in PIM list: " << pim->read_sreg(1) << endl;
    cout << "(main.cpp): PIM internal setup complete ... " << endl;

    cout << "(main.cpp): lower bound for keys -- " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): upper bound for keys -- " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): initial size of linked list -- " << INITIAL_LIST_SIZE << endl;

    cout << "(main.cpp): adding initial nodes to linked list ... " << endl;

#ifdef HOST_DEBUG_ON
	vector<ulong_t> initial_keys;
#endif
    while (randmap.size() < INITIAL_LIST_SIZE) {
        key_value = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        rand_iterations++;
        if (randmap.count(key_value) == 0) {
            randmap[key_value] = key_value;
            if ((randmap.size() % 1000) == 0) {
                cout << "reached " << randmap.size() << " items in initial randmap!!" << endl;
            }
        }
    }
    cout << "took " << rand_iterations << " rand calls to get " << INITIAL_LIST_SIZE << " unique elements" << endl;
    for (std::pair<ulong_t, ulong_t> element : randmap) {
            //cout << "add(" << element.first << ")" << endl;
            pim->write_parameter(element.first,0);
            pim->write_sreg(1, rand_level());
            SET_OPERATION(request_slot.info_bits, ADD_OP);
            pim->write_info_bits(request_slot.info_bits, 0);
            SET_REQUEST(request_slot.info_bits);
            pim->write_info_bits(request_slot.info_bits, 0);
            while (CHECK_VALID_REQUEST(request_slot.info_bits)) {
                request_slot.info_bits = pim->read_info_bits(0);
            }
            pim_res = (ulong_t) GET_RETVAL(request_slot.info_bits);
            if (pim_res == 1) {
                size++;
                if ((size % 1000) == 0) {
                    cout << "reached " << size << " items in initial list!!" << endl;
                }
    #ifdef HOST_DEBUG_ON
                //cout << "size: " << size << endl;
                initial_keys.push_back(element.first);
    #endif
            }
    }
    cout << "added total " << size << " elements to linked list!!" << endl;

    // reset operation timestamp counter on PIM
    pim->write_sreg(2,0);
    pim->write_sreg(0,1); // set PIM to ops mode

    cout << "(main.cpp): added " << size << " elements to linked list ... printing contents: " << endl;
#ifdef HOST_DEBUG_ON
	for (unsigned int i=0; i<initial_keys.size(); i++)
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
            thread_data[i].pimapi = pim;
            thread_data[i].num_ops = (TOTAL_NUM_OPS / num_threads);
            thread_data[i].rand_keys = (ulong_t *)malloc(sizeof(ulong_t) * (TOTAL_NUM_OPS / num_threads));
            thread_data[i].rand_ops = (ulong_t *)malloc(sizeof(ulong_t) * (TOTAL_NUM_OPS / num_threads));
            for (j = 0; j < (TOTAL_NUM_OPS / num_threads); j++) {
                thread_data[i].rand_keys[j] = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
                thread_data[i].rand_ops[j] = rand() % 100;
            }
        }

        for (i = 0; i < num_threads; i++) {
            pthread_attr_init(&(thread_attr[i]));
            CPU_ZERO(&cpus);
            CPU_SET(i, &cpus);
            pthread_attr_setaffinity_np(&(thread_attr[i]), sizeof(cpu_set_t), &cpus);
            thread_create_ret = pthread_create(&(threads[i]), &(thread_attr[i]), linkedlist_ops_routine, (void *)(&(thread_data[i])));
            if (thread_create_ret != 0) {
                cout << "(main.cc): Error in creating list routine thread, threadID " << i << ": failed with error number " << thread_create_ret << endl;
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
        
        pim->write_sreg(2,0); // reset operation timestamp
        
        for (i = 0; i < num_threads; i++) {
            free((void *)(thread_data[i].rand_keys));
            free((void *)(thread_data[i].rand_ops));
        }
    }

    // close PIM kernel
    pim->write_sreg(0,4);
    pim->wait_for_completion();
    
#ifdef HOST_DEBUG_ON
    pthread_mutex_destroy(&mylock);
#endif
    

    APP_INFO("[---DONE---]")


    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

