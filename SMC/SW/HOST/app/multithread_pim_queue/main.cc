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
    PIMAPI **pimapi;
    ulong_t *rand_keys;
    int num_ops;
} thread_data_t;

char start_threads;

int enq_part; 
int deq_part;

#define GET_NEXT_PARTITION(_x) ((_x + 1) % NUM_PARTITIONS)

 
void * ops_routine(void *routine_args) 
{
    int thread_id = ((thread_data_t *)routine_args)->thread_id;
    PIMAPI **pim = ((thread_data_t *)routine_args)->pimapi;
    ulong_t *rand_keys = ((thread_data_t *)routine_args)->rand_keys;
    request_slot_t request_slot;
    request_slot.info_bits = 0;
    request_slot.timestamp = 0;
    request_slot.parameter = 0;
    int op_count = 0;
    int num_ops = ((thread_data_t *)routine_args)->num_ops;
    uint16_t valid = 0;
    int curr_enq_part = enq_part;
    int curr_deq_part = deq_part;

#ifdef HOST_DEBUG_ON
	vector<string> operation_type;
	vector<int>    operation_args;
	vector<int>    operation_part;
    vector<int>    operation_timestamp;
#endif

    while (start_threads == 0) { /*wait*/ }

    while (op_count < num_ops) {
        request_slot.parameter = rand_keys[op_count];

        //if (rand_ops[op_count] < PUSH_PERCENTAGE) {
        if ((op_count + thread_id) % 2 == 0) {
            //enq
            curr_enq_part = enq_part;
            pim[curr_enq_part]->write_parameter(request_slot.parameter, thread_id);
            SET_REQUEST(request_slot.info_bits);
            pim[curr_enq_part]->write_info_bits(request_slot.info_bits, thread_id);

            do {
                request_slot.info_bits = pim[curr_enq_part]->read_info_bits(thread_id);
                valid = CHECK_VALID_REQUEST(request_slot.info_bits) * CHECK_VALID_PIM(request_slot.info_bits);
            } while ((valid != 0) && (curr_enq_part == enq_part));

            if (curr_enq_part != enq_part) {
                // read info bits from pim one last time, to get finalized state of request
                request_slot.info_bits = pim[curr_enq_part]->read_info_bits(thread_id);
            }

            if (CHECK_VALID_REQUEST(request_slot.info_bits) == 0) {
                // request completed normally
                op_count++;
#ifdef HOST_DEBUG_ON
                operation_type.push_back("push");
                operation_args.push_back(request_slot.parameter);
                operation_part.push_back(curr_enq_part);
                request_slot.timestamp = pim[curr_enq_part]->read_timestamp(thread_id);
                operation_timestamp.push_back(request_slot.timestamp);
#endif
            } else {
                // operation needs to move to next partition.
                // clear the current partition request slot, 
                // so that future operations on the partition are not affected
                CLEAR_REQUEST(request_slot.info_bits);
                pim[curr_enq_part]->write_info_bits(request_slot.info_bits, thread_id);
            }

            if (CHECK_VALID_PIM(request_slot.info_bits) == 0) {
                // this thread has been designated as the partition-changer
                enq_part = GET_NEXT_PARTITION(curr_enq_part);
#ifdef HOST_DEBUG_ON
                //cout << "thread " << thread_id << ": changing enq partition to " << enq_part << endl;
#endif
                pim[enq_part]->write_sreg(0,1);
                pim[enq_part]->start_computation(PIMAPI::CMD_RUN_KERNEL);
                pim[curr_enq_part]->wait_for_completion_simple(); // close previous partition
            }

        } else {
            //deq
            curr_deq_part = deq_part;
            SET_REQUEST(request_slot.info_bits);
            pim[curr_deq_part]->write_info_bits(request_slot.info_bits, thread_id);

            do {
                request_slot.info_bits = pim[curr_deq_part]->read_info_bits(thread_id);
                valid = CHECK_VALID_REQUEST(request_slot.info_bits) * CHECK_VALID_PIM(request_slot.info_bits);
            } while ((valid != 0) && (curr_deq_part == deq_part));

            if (curr_deq_part != deq_part) {
                // read info bits from pim one last time, to get finalized state of request
                request_slot.info_bits = pim[curr_deq_part]->read_info_bits(thread_id);
            }

            if (CHECK_VALID_REQUEST(request_slot.info_bits) == 0) {
                // request completed normally
                request_slot.parameter = pim[curr_deq_part]->read_parameter(thread_id);
                op_count++;
#ifdef HOST_DEBUG_ON
                operation_type.push_back("pop");
                operation_args.push_back(request_slot.parameter);
                operation_part.push_back(curr_deq_part);
                request_slot.timestamp = pim[curr_deq_part]->read_timestamp(thread_id);
                operation_timestamp.push_back(request_slot.timestamp);
#endif
            } else {
                // operation needs to move to next partition.
                // clear the current partition request slot, 
                // so that future operations on the partition are not affected
                CLEAR_REQUEST(request_slot.info_bits);
                pim[curr_deq_part]->write_info_bits(request_slot.info_bits, thread_id);
            }

            if (CHECK_VALID_PIM(request_slot.info_bits) == 0) {
                // this thread has been designated as the partition-changer
                deq_part = GET_NEXT_PARTITION(curr_deq_part);
#ifdef HOST_DEBUG_ON
                //cout << "thread " << thread_id << ": changing deq partition to " << deq_part << endl;
#endif
                pim[deq_part]->write_sreg(0,2);
                pim[deq_part]->start_computation(PIMAPI::CMD_RUN_KERNEL);
                pim[curr_deq_part]->wait_for_completion_simple(); // close previous partition
            }
        }

    } // while op_count < num_ops

#ifdef HOST_DEBUG_ON
    pthread_mutex_lock(&mylock);
	for (unsigned int i = 0; i < operation_type.size(); i++) {
		cout << "Print_from_Thread_ID " << thread_id << ": " << operation_type[i] << " ";
		cout << operation_args[i] << " partition: " << operation_part[i] << " timestamp: ";
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
    PIMAPI *pim[NUM_PARTITIONS] = {0};
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

    enq_part = 0;
    deq_part = 0;

#ifdef HOST_DEBUG_ON
    pthread_mutex_init(&mylock, NULL);
#endif

    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    cout << "(main.cpp): Offloading the computation kernel ... " << endl;

    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i] = new PIMAPI(i);
        pim[i]->offload_kernel((char *)FILE_NAME);
    }

    /* PIM scalar registers
    SREG[0]: PIM operation
                0 -- OFF 
                1 -- ENQ
                2 -- DEQ
                3 -- INIT
                4 -- PRINT
    SREG[1]: MAX_SEGMENT_BYTES (HOST->PIM)
    */

    // initialize PIM kernel for each partition
    for (i = 0; i < NUM_PARTITIONS; i++) {
        cout << "initializing partition " << i << endl;
        pim[i]->write_sreg(0,3); // init op
        pim[i]->write_sreg(1,MAX_SEGMENT_BYTES); // set max_segment_bytes
        pim[i]->start_computation(PIMAPI::CMD_RUN_KERNEL);
    }

    // wait for initialization to complete
    for (i = 0; i < NUM_PARTITIONS; i++) {
        pim[i]->wait_for_completion(); // this part takes a long time, so use traditional wait_for_completion
        cout << "initialization complete for partition" << i << endl;
    }

    cout << "(main.cpp): lower bound for keys -- " << KEY_LOWER_BOUND << endl;
    cout << "(main.cpp): upper bound for keys -- " << KEY_UPPER_BOUND << endl;
    cout << "(main.cpp): initial size of queue -- " << INITIAL_Q_SIZE << endl;
    cout << "(main.cpp): adding initial items to queue ... " << endl;

    // prepare initial queue
    pim[enq_part]->write_sreg(0,1); // enq op
    pim[enq_part]->start_computation(PIMAPI::CMD_RUN_KERNEL);

    while (size < INITIAL_Q_SIZE) {
        request_slot.parameter = (rand() % KEY_UPPER_BOUND) + KEY_LOWER_BOUND;
        pim[enq_part]->write_parameter(request_slot.parameter, 0);
#ifdef HOST_DEBUG_ON
        cout << "add " << request_slot.parameter << " to partition " << enq_part << endl;
#endif
        SET_REQUEST(request_slot.info_bits);
        pim[enq_part]->write_info_bits(request_slot.info_bits, 0);

        do {
            request_slot.info_bits = pim[enq_part]->read_info_bits(0);
            valid = CHECK_VALID_REQUEST(request_slot.info_bits) * CHECK_VALID_PIM(request_slot.info_bits);
        } while (valid != 0);

        if (CHECK_VALID_REQUEST(request_slot.info_bits) == 0) {
            // request completed normally
            size++;
#ifdef HOST_DEBUG_ON
            cout << "curr size " << size << endl;
#endif
            if (size % 10000 == 0) {
                cout << "added " << size << " items to initial queue; currently working in partition " << enq_part << endl;
            }
        }

        if (CHECK_VALID_PIM(request_slot.info_bits) == 0) {
            // need to move onto next partition
            // this notification comes to only one thread (the thread associated with the last )
            pim[enq_part]->wait_for_completion_simple();
            enq_part++; // use non-atomic operation since this part is single-threaded anyway
                        // (and just ++, since I know that it's not going to overflow back to 0)
            cout << "moving on to partition " << enq_part << endl;
            pim[enq_part]->write_sreg(0,1);
            pim[enq_part]->start_computation(PIMAPI::CMD_RUN_KERNEL);
        }
    } // while size < INITIAL_Q_SIZE

    cout << "(main.cpp): completed initialization .. close currently active PIM ... " << endl;
    pim[enq_part]->write_sreg(0,0);
    pim[enq_part]->wait_for_completion_simple();

    cout << "(main.cpp): printing contents of queue ... " << endl;
    for (i = 0; i <= enq_part; i++) {
        pim[i]->write_sreg(6,0); //reset timestamp
#ifdef HOST_DEBUG_ON
        pim[i]->write_sreg(0,4);
        pim[i]->start_computation(PIMAPI::CMD_RUN_KERNEL);
        pim[i]->wait_for_completion(); 
#endif
    }


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

        // activate the first enq and deq partitions here.
        // when partitions get emptied-out and filled-up, 
        // the ops routine threads will deactivate and reactivate partitions as necessary.
        pim[enq_part]->write_sreg(0,1); //ENQ
        pim[enq_part]->start_computation(PIMAPI::CMD_RUN_KERNEL);
        pim[deq_part]->write_sreg(0,2); //DEQ
        pim[deq_part]->start_computation(PIMAPI::CMD_RUN_KERNEL);

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
        ____TIME_STAMP_PIMNO(0,(2*x-1));
        //____TIME_STAMP_PIMNO(0,1);
        start_threads = 1;
        // join list operation threads
        for (i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
        }
        ____TIME_STAMP_PIMNO(0,(2*x));
        //____TIME_STAMP_PIMNO(0,2);
        cout << "(main.cc): Done with " << num_threads << " threads!" << endl;
        
        // deactivate PIMs for now
        pim[enq_part]->write_sreg(0,0);
        pim[deq_part]->write_sreg(0,0);
        pim[enq_part]->wait_for_completion_simple();
        pim[deq_part]->wait_for_completion_simple();

        // free allocated memory
        for (i = 0; i < num_threads; i++) {
            free((void *)(thread_data[i].rand_keys));
//            free((void *)(thread_data[i].rand_ops));
        }

        cout << "print contents of queue ... " << endl;
        i = deq_part;
        do {
            pim[i]->write_sreg(6,0); // reset timestamp
    #ifdef HOST_DEBUG_ON
            pim[i]->write_sreg(0,4);
            pim[i]->start_computation(PIMAPI::CMD_RUN_KERNEL);
            pim[i]->wait_for_completion();
    #endif
            i = GET_NEXT_PARTITION(i);
        } while (i != deq_part);
    } // for x
    
#ifdef HOST_DEBUG_ON
    pthread_mutex_destroy(&mylock);
#endif
    

    APP_INFO("[---DONE---]")


    cout << "Exiting gem5 ..." << endl;
    pim[0]->give_m5_command(PIMAPI::M5_EXIT);
    return 0;
}

