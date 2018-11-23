/**
   Georgia Ma
   V00849447
   CSC 360
   Assignment 3 part 3
**/

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SECTOR_SIZE 512
#define MAX_INPUT 256

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Error: disk image and file name needed as argument.\n");
		exit(1);
	}
	// open disk image and map memory
	char *memblock;
	int fd;
	struct stat buff;
	
	fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error: could not open disk image.\n");
		exit(1);
	}
	
	fstat(fd, &buff);
	memblock = mmap(NULL, buff.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (memblock == MAP_FAILED) {
		printf("Error: could not map memory.\n");
		close(fd);
		exit(1);
	}
	
	
	// HERE
	
	
	// clean up
	munmap(memblock, buff.st_size);
	close(fd);
	
	return 0;

}