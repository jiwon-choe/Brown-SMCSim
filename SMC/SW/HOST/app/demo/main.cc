#include "pim_api.hh"
#include <unistd.h>
#include <iostream>
#include <unistd.h>

/*
This is a demo code
it does not deal with address translation and pointer passing
it just sends two vectors directly to PIM and asks it to add them.
*/

using namespace std;

int main(int argc, char *argv[])
{
	PIMAPI* pim = new PIMAPI();
	cout << "(main.cpp): Hello from user application!" << endl;
	
	uint8_t A[8];
	for ( int i=0; i<8; i++ )
	{
		A[i] = 1;
	}

	cout << "Testing scalar registers ..." << endl;

	for ( int i=0; i<4; i++ )
	{
		pim->write_sreg(i, 0xcafe0000 + i);
		cout << "SREG[" << i << "]=" << hex << pim->read_sreg (i) << endl;
	}
	
	cout << "Testing demo functionality: Increment(A)" << endl;

	for (int i=0; i< 50; i++)
	{
		pim->write_vreg(A);
		pim->start_computation(PIMAPI::CMD_DEMO);
		pim->wait_for_completion();
		pim->read_vreg(A);
		
		cout << "Result: ";
		for ( int i=0; i<8; i++ )
		{
			cout << dec << (int)A[i] << ",";
		}
		cout << endl;
		cout << "[--DONE--]" << endl;
		sleep(1);
	}
	
	return 0;
}
