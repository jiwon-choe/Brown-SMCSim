#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

using namespace std;

#define WALK_STEP 256
#define COUNT 1024*1024*2

// Main
int main(int argc, char *argv[])
{
    cout << "Killer app is executing ..." << endl;
    srand(0);

    unsigned long *A = new unsigned long[COUNT];
    unsigned long*B  = new unsigned long[COUNT];
    while (true)
    {
        memcpy(A, B, COUNT*sizeof(unsigned long));
        memcpy(B, A, COUNT*sizeof(unsigned long));
    }
	return 0;
}

