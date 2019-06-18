#ifndef CREATE_GRAPH_TOPOLOGY
#define CREATE_GRAPH_TOPOLOGY

#define MIN(a,b) ((a<b)?a:b)

void create_topology()
{
    srand(RANDOM_SEED);
    // Initialize the list of lists
    // Later, we should read the graph from data sets
    nodes = (node*)malloc(NODES*sizeof(node));
    assert (nodes);
    successors_list = NULL;

    unsigned long start = 0;
    unsigned long remaining = NODES;
    unsigned long component_size = 0;
    unsigned long component_degree = 0;
    #ifdef GRAPH_STATS
    unsigned long num_edges=0;
    #endif
    for (unsigned long c=0; c < NUM_COMPONENTS; c++ )
    {
        if ( remaining == 0 )
            continue;

        if ( c == NUM_COMPONENTS-1 )
            component_size = remaining;
        else if ( NUM_COMPONENTS >= 3 )
            component_size = rand()%(remaining/(NUM_COMPONENTS/3))+1;
        else
            component_size = rand()%(remaining/(NUM_COMPONENTS))+1;

        component_degree = MIN( MAX_COMPONENT_OUTDEGREE, component_size );
        #ifdef GRAPH_STATS
        num_edges = 0;
        #endif
        for ( unsigned long n=start; n<start+component_size; n++ )
        {
            unsigned long num_succ = MIN( rand()%(component_degree+1), component_size-1);
            nodes[n].out_degree = num_succ;
            nodes[n].ID = n;
            #ifdef GRAPH_STATS
            num_edges += num_succ;
            #endif
            if ( num_succ == 0 )
                nodes[n].successors = NULL;
            else
            {
                nodes[n].successors = (node**)malloc(num_succ*sizeof(node*)); 
                assert (nodes[n].successors);
            }

            /* fill the successor list of node[i] */
            unsigned long succ = start + rand()%component_size; // Random;
            for ( unsigned long j=0; j<num_succ; j++ )
            {
                // No loop is allowed in the graph
                if ( succ == n ) succ++;
                if ( succ >= start+component_size ) succ = start;
                if ( succ == n ) succ++;
                /* we have found a new successor which we are not already connected to */
                nodes[n].successors[j] = &nodes[succ];
                #ifdef DEBUG
                //printf("  node %ld --> %ld\n", n, succ );
                assert( n != succ );
                #endif
                succ ++;
            }
        }
        #ifdef GRAPH_STATS
        printf("Component %4ld -- Size: %8ld -- Start: %8ld -- End: %8ld -- MaxDegree: %4ld -- Links: %8ld \n",
            c, component_size, start, start+component_size-1, component_degree, num_edges );
        #else
        printf("Component %4ld -- Size: %8ld -- Start: %8ld -- End: %8ld -- MaxDegree: %4ld \n",
            c, component_size, start, start+component_size-1, component_degree );
        #endif

        remaining -=component_size;
        start += component_size;
    }

    /* Replace some of the local links with global links */
    printf("Adding %d global links\n", NUM_GLOBAL_EDGES );
    unsigned long n = 0;
    unsigned long succ = 0;
    for (unsigned long i=0; i<NUM_GLOBAL_EDGES; i++)
    {
        char found = 1;
        while (found == 1 || nodes[n].out_degree == 0)
        {
            found = 0;
            n = rand()%NODES;
            succ = rand()%NODES;
            while ( succ == n )
                succ = rand()%NODES;

            /* Check if the connection is already present */
            for (unsigned long j=0; j< nodes[n].out_degree; j++)
                if ( nodes[n].successors[j]->ID == succ )
                {
                    found = 1;
                    #ifdef DEBUG
                    printf("    already connected: %ld --> %ld\n", n, succ );
                    #endif                    
                }
        }
        nodes[n].successors[0] = &nodes[succ]; // replace the first successor

        #ifdef DEBUG
        printf("  global edge %ld --> %ld\n", n, succ );
        #endif
    }


    successors_list = nodes[0].successors;

    #ifdef DUMP_GRAPH
    printf("Dumping graph to graph.csv ...\n" );
    FILE *f;
    f = fopen("graph.csv", "w");

    for ( unsigned long r=0; r<NODES; r++ )
    {
        for ( unsigned long c=0; c<nodes[r].out_degree ; c++ )
        {
            node*succ = nodes[r].successors[c];
            fprintf(f, "%ld, %ld\n", r, succ->ID);
        }
    }
    fclose(f);
    #endif
}

#endif
