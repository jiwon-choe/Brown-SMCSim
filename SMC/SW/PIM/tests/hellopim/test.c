#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char PIM_DEBUG_REG; // This is a special location in the memory for displaying debug messages
char msg1[50];
char msg2[50];
// 
void PRINT_DEBUG_MESSAGE( char* msg )
{
	int i=0;
	PIM_DEBUG_REG = '[';
	PIM_DEBUG_REG = 'P';
	PIM_DEBUG_REG = 'I';
	PIM_DEBUG_REG = 'M';
	PIM_DEBUG_REG = ']';
	PIM_DEBUG_REG = ':';
	while ( msg[i] != 0 )
	{
		PIM_DEBUG_REG = msg[i];
		i++;
	}
	PIM_DEBUG_REG = '\r';
	PIM_DEBUG_REG = '\n';
}

int mult_by_2(int a)
{
	return a*2;
}

int wait()
{
	long i=0;
	long SUM=0;
	for ( i=0; i<2000; i++)
		SUM+=i;
	return SUM;
}

void c_entry() {
	int arr[30];
	int arr2[30];
	int k = 0;
	int j = 7;
	int i = 0;
	sprintf(msg1,"First Message");
	sprintf(msg2,"Second Message");
	while (1)
	{
		PRINT_DEBUG_MESSAGE("................ Hello from the PIM device.");
		PRINT_DEBUG_MESSAGE(msg1);
		PRINT_DEBUG_MESSAGE(msg2);
		// for ( i=0; i<10; i++)
		// {
		// 	k = i % j;
		// 	wait();
		// 	arr[i] = (0xCAFE0000 + mult_by_2(i));
		// 	arr2[i] = arr[i] * 2;
		// }
		PRINT_DEBUG_MESSAGE("Now going to sleep");
		PRINT_DEBUG_MESSAGE("[SLEEP]");
		__asm__("wfi");
		PRINT_DEBUG_MESSAGE("[WAKE-UP]");
	}
	PRINT_DEBUG_MESSAGE("................ Goodbye from the PIM device. Crashing the system ...");
}