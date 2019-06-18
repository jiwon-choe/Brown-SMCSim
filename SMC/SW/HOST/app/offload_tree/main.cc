#include "pim_api.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <stdlib.h>
#include "_app_params.h"
#include "app_utils.hh"
using namespace std;
#include "utils.hh"
#include "defs.hh"

/*
This is a balanced binary search tree
*/

node* tree;                 // This does not need to be alighed to cache boundaries
unsigned long tree_size;    // Size of the allocated memory for the tree
static unsigned long num_nodes = 0;
ulong_t checksum = 0;

//////////////////////////////////////////////////////////////////////////////////////
// Tree Utils
//////////////////////////////////////////////////////////////////////////////////////

/*
This will ensure that the nodes of each row are consecutive in memory:
ROW 0 : NODE
ROW 1 : NODE NODE
ROW 2 : NODE NODE NODE NODE
*/
void create_tree()
{
    node* cn = NULL;
    node* pn = NULL;
    num_nodes = pow2(HEIGHT)-1;
    tree_size = num_nodes * sizeof(node);
    cout << "(main.cpp) Tree Size: " << tree_size << " Bytes" << endl;
    for ( unsigned long i=0; i<HEIGHT; i++)
    {
        cn = (node*)allocate_region(pow2(i)*sizeof(node));
        if ( i == 0 )
            tree = &cn[0];
        else
        {
            for ( unsigned long j=0; j<pow2(i-1); j++ )
            {
                pn[j].left = &cn[j*2];
                pn[j].right = &cn[j*2+1];
            }
        }            
        pn = cn;
    }
}

// Initialize the Binary Search Tree
void init_tree(node* current, unsigned long start, unsigned long count )
{
    if ( current == NULL )
        return;
    init_tree(current->left, start, (count-1)/2);
    init_tree(current->right, start+(count-1)/2+1, (count-1)/2);
    current->key = start+count/2;
}

void print_recursive(node* n)
{
    if ( !n )
        return;
    cout << "N= " << n->key;
    if ( n->left )
        cout << " L=" << n->left->key;
    if ( n->right )
        cout << " R=" << n->right->key;
    cout << endl;
    print_recursive(n->left);
    print_recursive(n->right);
}

// Only for small trees
void print_tree()
{
    node* cn = tree;
    int space = 64;
    for ( unsigned long i=0; i<HEIGHT; i++)
    {
        if ( i == 0 )
        {
            for (int k=0; k<space; k++) cout << " ";
            cout << cn->key << endl;
        }
        else
        {
            for (int k=0; k<space; k++) cout << " ";
            for ( unsigned long j=0; j<pow2(i); j++ )
            {
                cout << cn[j].key;
                for (int k=0; k<space*2; k++) cout << " ";
            }
            cout << endl;
        }
        if ( space > 1 )
            space = (int)((float)space * 0.5);
        cn = cn + pow2(i);
    }

    //print_recursive(tree);
}

bool check_tree(node* n)
{
    if ( ! n )
        return false;
    else
    {
        bool flag = true;
        if ( n->left )
        {
            if ( n->left->key >= n->key )
            {
                cout << "Error in BST structure!" << endl;
                flag = false;
            }
            if ( flag ) 
                flag = check_tree(n->left);
        }
        if ( n->right )
        {
            if ( n->right->key <= n->key )
            {
                cout << "Error in BST structure!" << endl;
                flag = false;
            }
            if ( flag )
                flag = check_tree(n->right);
        }
        return flag;
    }
}

//////////////////////////////////////////////////////////////////////////////////////
// Search List
//////////////////////////////////////////////////////////////////////////////////////
ulong_t* search_list;
void init_search_list()
{
    search_list = (ulong_t*)allocate_region(NUM_OPERATIONS*sizeof(ulong_t));
    for ( int i=0; i<NUM_OPERATIONS; i++ )
    {
        search_list[i] = rand()%num_nodes;
        //cout << search_list[i] << endl;
    }
}

// Golden model for binary search
//  Runs N searches on the tree
void golden_search()
{
    checksum = 0;
    for ( int i=0; i<NUM_OPERATIONS; i++ )
    {
        node* curr = tree;
        while (curr)
        {
            checksum++;
            if ( curr-> key == search_list[i] )
            {
                //APP_INFO("Found key: %d", curr->key );
                break;
            }
            else
            if ( curr-> key > search_list[i] )
                curr = curr->left;
            else
                curr = curr-> right;
        }
        if ( !curr )
        {
            cout << "Error: Node was not found! " << search_list[i] << endl;
            exit(1);
        }

    }
}

/************************************************/
// Main
int main(int argc, char *argv[])
{
    init_region();
    PIMAPI *pim = new PIMAPI(); /* Instatiate the PIM API */ 
    cout << "(main.cpp): Kernel Name: " << FILE_NAME << endl;
    //cout << "(main.cpp): Create the tree with height " << HEIGHT << " ..." << endl;
    create_tree();

    cout << "(main.cpp): Initializing the tree ... ";
    init_tree(tree, 0, num_nodes);
    cout << num_nodes << " nodes were initialized ..." << endl;
    //print_tree();

    #ifdef DEBUG_APP
    cout << "(main.cpp): Checking the tree ..." << endl;
    if (! check_tree(tree))
        cout << "(main.cpp): Error in the tree!" << endl;
    #endif

    cout << "(main.cpp): Initializing the search list ... " << endl;
    init_search_list();

    cout << "(main.cpp): Offloading the computation kernel ... " << endl;
    ____TIME_STAMP(1);
    #ifdef OFFLOAD_THE_KERNEL
    pim->offload_kernel((char*)FILE_NAME);
    #else
    cout << "Kernel offloading skipped! (See the OFFLOAD_THE_KERNEL environment variable)" << endl;
    #endif
    ____TIME_STAMP(2);

	/* PIM Scalar Registers
	SREG[0]: Tree
	SREG[1]: Search List
    SREG[2]: Number of searches
    SREG[3]: Checksum (returned from PIM)
	*/

    ____TIME_STAMP(3);
	// Create and assign the data set for this task
	PIMTask* task = new PIMTask("tree-search-task");
	task->addData(tree, tree_size, 0);
	task->addData(search_list, NUM_OPERATIONS*sizeof(ulong_t), 1);
	pim->write_sreg(2, NUM_OPERATIONS);  // This is a parameter sent to PIM

 //    /* Enable packet logging */
 //    //pim->give_m5_command(PIMAPI::M5_ENABLE_PACKET_LOG);

    cout << "(main.cpp): Offloading the task ... " << endl;
	pim->offload_task(task);
    ____TIME_STAMP(4);


    cout << "(main.cpp): Start computation ... " << endl;
    ____TIME_STAMP(5);
	pim->start_computation(PIMAPI::CMD_RUN_KERNEL);
	pim->wait_for_completion();
    //____TIME_STAMP(6); // This is done in the resident code

 //    /* Disable packet logging */
 //    //pim->give_m5_command(PIMAPI::M5_DISABLE_PACKET_LOG);

    APP_INFO("[---DONE---]")

    cout << "(main.cpp): Running the golden model ... " << endl;
    ____TIME_STAMP(7);
    golden_search();
    ____TIME_STAMP(8);
    cout << "Checksum = " << hex << checksum << dec << endl;


    if ( pim->read_sreg(3) == checksum ) // Success
        print_happy();
    else
        cout << "Error! something went wrong ..." << endl;


    pim->report_statistics();
    cout << "Exiting gem5 ..." << endl;
    pim->give_m5_command(PIMAPI::M5_EXIT);
    // cout << "(main.cpp): Exiting main application!" << endl;
    // delete pim;
	return 0;
}

