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
#ifdef HOST_DEBUG_ON
#include <vector>
#endif
using namespace std;


#ifdef HOST_DEBUG_ON
pthread_mutex_t mylock;
#endif

typedef struct thread_data {
    int thread_id;
    PIMAPI *pimapi;
    ulong_t *rand_keys;
    int num_ops;
} thread_data_t;

char start_threads;


 
void * ops_routine(void *routine_args) 
{
    int thread_id = ((thread_data_t *)routine_args)->thread_id;
    PIMAPI *pim = ((thread_data_t *)routine_args)->pimapi;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    request_slot_t request_slot;
    request_slot.info_bits = 0;
    request_slot.timestamp = 0;
    request_slot.parameter = 0;
    int op_count = 0;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    uint16_t valid = 0;

#ifdef HOST_DEBUG_ON
	vector<string> operation_type;
	vector<int>    operation_args;
    vector<int>    operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {
        request_slot.parameter = rand_keys[op_count];

        //if (rand_ops[op_count] < PUSH_PERCENTAGE) {
        if ((op_count + thread_id) % 2 == 0) {
            //enq
            pim->write_parameter(request_slot.parameter, thread_id);
            SET_ENQ(request_slot.info_bits);
            SET_REQUEST(request_slot.info_bits);
            pim->write_info_bits(request_slot.info_bits, thread_id);

            do {
                request_slot.info_bits = pim->read_info_bits(thread_id);
                valid = CHECK_VALID_REQUEST(request_slot.info_bits);
            } while (valid != 0);

            // request completed normally
            op_count++;
#ifdef HOST_DEBUG_ON
            operation_type.push_back("push");
            operation_args.push_back(request_slot.parameter);
            request_slot.timestamp = pim->read_timestamp(thread_id);
            operation_timestamp.push_back(request_slot.timestamp);
#endif
        } else {
            //deq
            SET_DEQ(request_slot.info_bits);
            SET_REQUEST(request_slot.info_bits);
            pim->write_info_bits(request_slot.info_bits, thread_id);

            do {
                request_slot.info_bits = pim->read_info_bits(thread_id);
                valid = CHECK_VALID_REQUEST(request_slot.info_bits);
            } while (valid != 0);

            // request completed normally
            request_slot.parameter = pim->read_parameter(thread_id);
            op_count++;
#ifdef HOST_DEBUG_ON
            operation_type.push_back("pop");
            operation_args.push_back(request_slot.parameter);
            request_slot.timestamp = pim->read_timestamp(thread_id);
            operation_timestamp.push_back(request_slot.timestamp);
#endif
        }

    } // while op_count < num_ops

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&mylock);
	for (unsigned int i = 0; i < operation_type.size(); i++) {
		cout << "Print_from_Thread_ID " << thread_id << ": " << operation_type[i] << " ";
		cout << operation_args[i] << " timestamp: ";
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
    PIMAPI *pim = new PIMAPI();
    request_slot_t request_slot;
    request_slot.info_bits = 0;
    request_slot.timestamp = 0;
    request_slot.parameter = 0;
    pthread_t threads[NUM_HOST_THREADS] = {0};
    thread_data_t thread_data[NUM_HOST_THREADS] = {0};
    pthread_attr_t thread_attr[NUM_HOST_THREADS];
    cpu_set_t cpus;

    int i = 0;
    int j = 0;
    int x = 0;
    int num_threads = 0;
    int size = 0;
    int ret = 0;
    uint16_t valid = 0;

    srand(time(NULL));          // seed the RNG with time

#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&mylock, NULL);
#endif

    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;

    pim->offload_kernel((char *)FILE_NAME);

    /* PIM scalar registers
    SREG[0]: PIM operation
                0 -- OFF 
                1 -- ENQ/DEQ
                3 -- INIT
                4 -- PRINT
    */

    // initialize PIM kernel
    cout << "initializing partition " << i << endl;
    pim->write_sreg(0,3); // init op
    pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
    cout << "initialization complete for partition" << i << endl;
    pim->wait_for_completion(); // this part takes a long time, so use traditional wait_for_completion

    cout << "(main.cpp): lower bound for keys -- " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): upper bound for keys -- " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): initial size of queue -- " << INITIAL_Q_SIZE << endl;
    cout << "(main.cpp): adding initial items to queue ... " << endl;

    // prepare initial queue
    pim->write_sreg(0,1); // enq op
    pim->start_computation(PIMAPI::CMD_RUN_KERNEL);

    while (size < INITIAL_Q_SIZE) {
        request_slot.parameter = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        pim->write_parameter(request_slot.parameter, 0);
#ifdef HOST_DEBUG_ON
        cout << "add " << request_slot.parameter << endl;
#endif
        SET_ENQ(request_slot.info_bits);
        SET_REQUEST(request_slot.info_bits);
        pim->write_info_bits(request_slot.info_bits, 0);

        do {
            request_slot.info_bits = pim->read_info_bits(0);
            valid = CHECK_VALID_REQUEST(request_slot.info_bits);
        } while (valid != 0);

        // request completed normally
        size++;
#ifdef HOST_DEBUG_ON
        cout << "curr size " << size << endl;
#endif
        if (size % 10000 == 0) {
            cout << "added " << size << " items to initial queue" << endl;
        }

    } // while size < INITIAL_Q_SIZE

    cout << "(main.cpp): completed initialization .. close currently active PIM ... " << endl;
    pim->write_sreg(0,0);
    pim->wait_for_completion_simple();

    cout << "(main.cpp): printing contents of queue ... " << endl;
    pim->write_sreg(6,0); //reset timestamp
#ifdef HOST_DEBUG_ON
    pim->write_sreg(0,4);
    pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
    pim->wait_for_completion(); 
#endif


    cout << "(main.cpp): testing with a total of " << TOTAL_NUM_OPS << " operations." << endl;
    cout << "(main.cpp): percentage of push operations is ";
    cout << PUSH_PERCENTAGE << endl;

    for (x = 1; x <= 4; x++) {
        num_threads = 2*x;
        start_threads = 0;

        for (i = 0; i < num_threads; i++) {
            thread_data[i].thread_id = i;
            thread_data[i].pimapi = pim;
            thread_data[i].num_ops = (TOTAL_NUM_OPS / num_threads);
            thread_data[i].rand_keys = (ulong_t *)malloc(sizeof(ulong_t) * (TOTAL_NUM_OPS / num_threads));
            for (j = 0; j < (TOTAL_NUM_OPS / num_threads); j++) {
                thread_data[i].rand_keys[j] = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
            }
            pthread_attr_init(&(thread_attr[i]));
            CPU_ZERO(&cpus);
            CPU_SET(i, &cpus);
            pthread_attr_setaffinity_np(&(thread_attr[i]), sizeof(cpu_set_t), &cpus);
        }

        pim->write_sreg(0,1); //ENQ DEQ
        pim->start_computation(PIMAPI::CMD_RUN_KERNEL);

        for (i = 0; i < num_threads; i++) {
            ret = pthread_create(&(threads[i]), &(thread_attr[i]), ops_routine, (void *)(&(thread_data[i])));
            if (ret != 0) {
                cout << "(main.cc): Error in creating list routine thread, threadID " << i << ": failed with error number " << ret << endl;
                return -1;
            }
        }

        for (i = 0; i < 10000; i++) {
            // wait
        }
        ____TIME_STAMP((2*x-1));
        start_threads = 1;
        // join list operation threads
        for (i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        ____TIME_STAMP(2*x);
        cout << "(main.cc): Done with " << num_threads << " threads!" << endl;
        
        // deactivate PIMs for now
        pim->write_sreg(0,0);
        pim->wait_for_completion_simple();

        // free allocated memory
        for (i = 0; i < num_threads; i++) {
            free((void *)(thread_data[i].rand_keys));
//            free((void *)(thread_data[i].rand_ops));
        }

        cout << "print contents of queue ... " << endl;
        pim->write_sreg(6,0); // reset timestamp
    #ifdef HOST_DEBUG_ON
        pim->write_sreg(0,4);
        pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
        pim->wait_for_completion();
    #endif
    } // for x
    
#ifdef HOST_DEBUG_ON
    pthread_mutex_destroy(&mylock);
#endif
    

    APP_INFO("[---DONE---]")


    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

