
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>

// Erfan
#include <unistd.h>
#include "_params.h"

int main(int argc, char **argv) {
  
	int fd;
	int a, b;
	
	printf("TestDriver: Page Size = %d\n", getpagesize());
	
	fd = open("/dev/PIM", O_RDWR | O_SYNC);
	if (fd < 0)
		perror("Open /dev/PIM");

	//mmap the whole physical range to userspace
	void * p = mmap(NULL, PHY_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, PHY_BASE); 
	
	if(p == (void *) -1) {
	  printf("Memory map failed.\n");
	  perror("mmap");
	} else {
	  printf("Memory mapped at address %p.\n", p);
	}
	  
	//write/read the region just mmapped
	((char*)(p))[0] = 10;
	((char*)(p))[1] = 20;
	((char*)(p))[2] = 30;
	((char*)(p))[3] = 40;
	printf("Writing was successful\n");

	char rd[4];
	rd[0] = ((char*)(p))[0];
	rd[1] = ((char*)(p))[1];
	rd[2] = ((char*)(p))[2];
	rd[3] = ((char*)(p))[3];

	printf("Write = %d, %d, %d, %d\n", 10, 20, 30, 40);
	printf("Read  = %d, %d, %d, %d\n", rd[0], rd[1], rd[2], rd[3]);
	
	printf("Writing to PIM_INTR_REG\n");
	((char*)(p))[PIM_INTR_REG-PHY_BASE] = 40;
	printf("Done!\n");

	return 0;
}
