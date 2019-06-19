#include "pim_api.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include "app_utils.hh"     // This must be included before anything else
#include <stdlib.h>         // srand(), rand()
#include <time.h>           // time()
#include "utils.hh"
#include <unordered_map>
#ifdef HOST_DEBUG_ON
#include <vector>
#endif
using namespace std;


#ifdef HOST_DEBUG_ON
pthread_mutex_t mylock;
#endif

typedef struct thread_data {
    int partition_max_size;
    int max_level;
    int thread_id;
    ulong_t *rand_keys;
    ulong_t *rand_ops;
    int num_ops; 
    PIMAPI **pimapi; // pointer to PIMAPI pointers (pimapi is array of PIMAPI pointers)
} thread_data_t;

volatile char start_threads;


uint8_t random_level(int curr_max_level) {
    ulong_t x = rand() % (1 << curr_max_level);
    if ((x & 0x80000001) != 0) { // test highest and lowest bits
        return 1;
    }
    uint8_t level = 2;
    while (((x >>= 1) & 1) != 0) {
        level++;
    }
    if (level > (curr_max_level - 1)) {
        return (curr_max_level - 1);
    }
    return level;
}

void * skiplist_ops_routine(void *routine_args) 
{
    int partition_max_size = ((thread_data_t *)routine_args)->partition_max_size;
    int max_level = ((thread_data_t *)routine_args)->max_level;
    int thread_id = ((thread_data_t *)routine_args)->thread_id;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    ulong_t *rand_ops = ((thread_data_t *)routine_args)->rand_ops;
    PIMAPI **pim = ((thread_data_t *)routine_args)->pimapi;
    request_slot_t request_slot;
    request_slot.info_bits = 0;
    request_slot.random_level = 0;
    request_slot.timestamp = 0;
    request_slot.parameter = 0;
    int op_count = 0;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    int rand_op_type = 0;
    int pim_res = 0;
    int part_id = 0;
#ifdef HOST_DEBUG_ON
	vector<string> operation_type;
	vector<int>    operation_args;
	vector<int>    operation_rslt;
    vector<int>    operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {
        request_slot.parameter = rand_keys[op_count];
        rand_op_type = rand_ops[op_count];
        op_count++;
        part_id = (request_slot.parameter - KEY_LOWER_BOUND) / partition_max_size;
        pim[part_id]->write_parameter(request_slot.parameter, thread_id);

        if (rand_op_type <= READ_ONLY_PERCENTAGE) {
            // contains
#ifdef HOST_DEBUG_ON
			operation_type.push_back("contains");
#endif
            SET_OPERATION(request_slot.info_bits, CONTAINS_OP);
            pim[part_id]->write_skiplist_info_bits(request_slot.info_bits, thread_id);
        } else if (rand_op_type % 2 == 0) {
            // add
#ifdef HOST_DEBUG_ON
			operation_type.push_back("add");
#endif
            SET_OPERATION(request_slot.info_bits, ADD_OP);
            pim[part_id]->write_skiplist_info_bits(request_slot.info_bits, thread_id);
            request_slot.random_level = random_level(max_level);
            pim[part_id]->write_skiplist_random_level(request_slot.random_level, thread_id);
        } else {
            // remove
#ifdef HOST_DEBUG_ON
			operation_type.push_back("remove");
#endif
            SET_OPERATION(request_slot.info_bits, REMOVE_OP);
            pim[part_id]->write_skiplist_info_bits(request_slot.info_bits, thread_id);
        }
        SET_REQUEST(request_slot.info_bits);
        pim[part_id]->write_skiplist_info_bits(request_slot.info_bits, thread_id);
        while (CHECK_VALID_REQUEST(request_slot.info_bits)) {
            request_slot.info_bits = pim[part_id]->read_skiplist_info_bits(thread_id);
        }
        pim_res = GET_RETVAL(request_slot.info_bits);
#ifdef HOST_DEBUG_ON
        operation_args.push_back(request_slot.parameter);
        operation_rslt.push_back(pim_res);
        request_slot.timestamp = pim[part_id]->read_timestamp(thread_id);
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
    // NUM_PARTITIONS is the same as number of partitions
    PIMAPI *pim[NUM_PARTITIONS] = {0};
    request_slot_t request_slot;
    request_slot.info_bits = 0;
    request_slot.random_level = 0;
    request_slot.timestamp = 0;
    request_slot.parameter = 0;
    pthread_t threads[NUM_HOST_THREADS] = {0};
    thread_data_t thread_data[NUM_HOST_THREADS] = {0};
    pthread_attr_t thread_attr[NUM_HOST_THREADS];
    cpu_set_t cpus;
    int thread_create_ret = 0;
    std::unordered_map<ulong_t, ulong_t> randmap = {};
    int rand_iterations = 0;
    int key_value = 0;
    int i = 0;
    int j = 0;
    int x = 0;
    int num_threads = 0;
    uint8_t pim_res = 0;
    int size = 0;
    int partition_max_size = 0;
    int max_level = 0;
    int part_id = 0;
    srand(time(NULL));          // seed the RNG with time

#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&mylock, NULL);
#endif

    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Initializing PIMAPI and offloading the computation kernel ... " << endl;
    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i] = new PIMAPI(i);
        pim[i]->offload_kernel((char *)FILE_NAME);
    }

    partition_max_size = (KEY_UPPER_BOUND - KEY_LOWER_BOUND + 1) / NUM_PARTITIONS; // make sure to set key bounds and # of pim devices so that this is an integer

    max_level = MAX_LEVEL;
    for (i = 1; i < MAX_LEVEL; i++) {
        if ((INITIAL_LIST_SIZE / NUM_PARTITIONS) < (1 << i)) {
            max_level = i;
            break;
        }
    } // sets max_level to log_2(initial partition size)
    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i]->write_sreg(6,max_level);
    }

    /* PIM scalar registers
    SREG[0]: PIM operation
                0 -- init, 
                1 -- skip list operations (aka flat-combine)
                2 -- print list
    SREG[1]: operation completion flag (PIM -> HOST)
    SREG[6]: max level necessary for initial list size (HOST->PIM)
    */
    // checks for valid inputs

    cout << "(main.cpp): sizeof(request_slot_t) = " << sizeof(request_slot_t) << endl;

    cout << "(main.cpp): lower bound for keys -- " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): upper bound for keys -- " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): number of partitions -- " << NUM_PARTITIONS << endl;
    cout << "(main.cpp): maximum size of each partition -- " << partition_max_size << endl;
    cout << "(main.cpp): initial size of skiplist -- " << INITIAL_LIST_SIZE << endl;
    cout << "(main.cpp): adding initial nodes to skiplist ... " << endl;

#if 0 // this is used for smaller size skiplists; else section used for near-max size skiplists
    cout << "(main.cpp): Prepare PIM kernel for skiplist operations ... " << endl;
    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i]->write_sreg(1,0); // reset operation completion flag before starting anything
        pim[i]->write_sreg(0,0);
        pim[i]->start_computation(PIMAPI::CMD_RUN_KERNEL); // only place where start_computation is called
    }
    for (i = 0; i < NUM_PARTITIONS; i++) {
        while (pim[i]->read_sreg(1) == 0) {
            // wait
        }
        cout << "maximum # of nodes on vault " << i << ": " << pim[i]->read_sreg(1) << endl;
    }

    while (randmap.size() < INITIAL_LIST_SIZE) {
        key_value = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        rand_iterations++;
        if (randmap.count(key_value) == 0) {
            randmap[key_value] = key_value;
            if (randmap.size() % 10000 == 0) {
                cout << "reached " << randmap.size() << " items in initial randmap!" << endl;
            }
        }
    }
    cout << "took " << rand_iterations << " rand calls to get " << INITIAL_LIST_SIZE << " unique elements" << endl;

    SET_OPERATION(request_slot.info_bits, ADD_OP);
    for (std::pair<ulong_t, ulong_t> element : randmap) {
        request_slot.parameter = element.first;
        part_id = (request_slot.parameter - KEY_LOWER_BOUND) / partition_max_size;
        request_slot.random_level = random_level(max_level);
#ifdef HOST_DEBUG_ON
        cout << "adding " << request_slot.parameter << " to partition " << part_id << " at level " << request_slot.random_level << endl;
#endif
        pim[part_id]->write_parameter(request_slot.parameter, 0);
        pim[part_id]->write_skiplist_random_level(request_slot.random_level, 0);
        SET_REQUEST(request_slot.info_bits);
        pim[part_id]->write_skiplist_info_bits(request_slot.info_bits, 0);
        while (CHECK_VALID_REQUEST(request_slot.info_bits)) {
            request_slot.info_bits = pim[part_id]->read_skiplist_info_bits(0);
        }
        pim_res = GET_RETVAL(request_slot.info_bits);
        if (pim_res == 1) {
            size++;
            if (size % 10000 == 0) {
                cout << "reached " << size << " items in initial skiplist" << endl;
            }
        }
    }
#else
    for (i = 0; i < NUM_PARTITIONS; i++) {
        size = 0;
        cout << "(main.cpp): Prepare PIM kernel for skiplist operations ... " << endl;
        pim[i]->write_sreg(1,0); // reset operation completion flag before starting anything
        pim[i]->write_sreg(0,0);
        pim[i]->start_computation(PIMAPI::CMD_RUN_KERNEL); // only place where start_computation is called
        while (pim[i]->read_sreg(1) == 0) {
            // wait
        }
        cout << "maximum # of nodes on vault " << i << ": " << pim[i]->read_sreg(1) << endl;

        SET_OPERATION(request_slot.info_bits, ADD_OP);
        part_id = i;
        while (size < (INITIAL_LIST_SIZE / NUM_PARTITIONS)) {
            request_slot.parameter = (rand() % partition_max_size) + (i * partition_max_size) + KEY_LOWER_BOUND;
            request_slot.random_level = random_level(max_level);
            pim[part_id]->write_parameter(request_slot.parameter, 0);
            pim[part_id]->write_skiplist_random_level(request_slot.random_level, 0);
            SET_REQUEST(request_slot.info_bits);
            pim[part_id]->write_skiplist_info_bits(request_slot.info_bits, 0);
            while (CHECK_VALID_REQUEST(request_slot.info_bits)) {
                request_slot.info_bits = pim[part_id]->read_skiplist_info_bits(0);
            }
            pim_res = GET_RETVAL(request_slot.info_bits);
            if (pim_res == 1) {
                size++;
                if (size % 10000 == 0) {
                    cout << "reached " << size << " items in initial skiplist part " << part_id << endl;
                }
                #ifdef HOST_DEBUG_ON
                cout << "added element: " << element.first << ", partition: " << part_id << ", level: " << request_slot.random_level << endl;
                #endif
            }
        }

        // temporarily shut down pim (so that simulator can run faster)
        pim[i]->write_sreg(0,3); // close pim
        pim[i]->wait_for_completion();
    }

    // reactivate closed PIMs
    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i]->write_sreg(0,1); // skiplist ops
        pim[i]->start_computation(PIMAPI::CMD_RUN_KERNEL);
    }
#endif

    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i]->write_sreg(2,0); // reset timestamp
    }
	
    cout << "(main.cpp): testing with a total of " << TOTAL_NUM_OPS << " operations." << endl;
    cout << "(main.cpp): percentage of read-only (contains) operations is ";
    cout << READ_ONLY_PERCENTAGE << endl;

    for (x = 1; x <= 4; x++) {
        num_threads = 2*x;
        start_threads = 0;

        for (i = 0; i < num_threads; i++) {
            thread_data[i].partition_max_size = partition_max_size;
            thread_data[i].max_level = max_level;
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
            thread_create_ret = pthread_create(&(threads[i]), &(thread_attr[i]), skiplist_ops_routine, (void *)(&(thread_data[i])));
            if (thread_create_ret != 0) {
                cout << "(main.cc): Error in creating list routine thread id " << i << ": failed w/ errno " << thread_create_ret << endl;
                return -1;
            }
        }
        for (i = 0; i < 1000; i++) {
            // wait for all threads to initialize
        }
        ____TIME_STAMP_PIMNO(0,(2*x-1));
        start_threads = 1;
        for (i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        ____TIME_STAMP_PIMNO(0,(2*x));

        for (i = 0; i < NUM_PARTITIONS; i++) {
            pim[i]->write_sreg(2,0); // reset operation timestamp
        }
        for (i = 0; i < num_threads; i++) {
            free((void *)(thread_data[i].rand_keys));
            free((void *)(thread_data[i].rand_ops));
        }
    }

    // close PIM kernel
    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i]->write_sreg(0,4);
        pim[i]->wait_for_completion();
    }
    
#ifdef HOST_DEBUG_ON
    pthread_mutex_destroy(&mylock);
#endif
    

    APP_INFO("[---DONE---]")


    cout << "Exiting gem5 ..." << endl;
    pim[0]->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

