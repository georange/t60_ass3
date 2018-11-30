/**
   Georgia Ma
   V00849447
   CSC 360
   Assignment 3 part 4
**/

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define SECTOR_SIZE 512
#define MAX_INPUT 256

int get_fat(char* memblock, int i) {
	int entry = 0;
	int byte1 = 0;
	int byte2 = 0;
		
	// compute entry content
	if ((i % 2) == 0) {
		byte1 = memblock[SECTOR_SIZE + (int)((3*i)/2)+1] & 0b00001111;
		byte2 = memblock[SECTOR_SIZE + (int)((3*i)/2)] & 0b11111111;
		entry = (byte1 << 8) + byte2;
	} else {
		byte1 = memblock[SECTOR_SIZE + (int)((3*i)/2)] & 0b00001111;
		byte2 = memblock[SECTOR_SIZE + (int)((3*i)/2)+1] & 0b11111111;
		entry = (byte1 >> 4) + (byte2 << 4);
	}
	return entry;
}

int get_total_size(char* memblock) {
	int total_sectors = memblock[19] + (memblock[20] << 8);
	int total_size = total_sectors * SECTOR_SIZE;

	return total_size;
}

int get_free_size(char* memblock, int size) {
	int free_spaces = 0;

	// traverse the FAT table
	int i;
	for (i = 2; i < (size/SECTOR_SIZE); i++) {
		int entry = get_fat(memblock, i);
		
		// if entry is 0x000, it is free
		if (!entry) {
			free_spaces++;
		}
	}
	int free_space = free_spaces * SECTOR_SIZE;
	return free_space;
}

int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Error: disk image and file name (with/without location) needed as argument.\n");
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
	
	// parse for file and subdirectory names if given
	char* input = argv[2];
	char* file_name = argv[2];
	char* subdirectories[MAX_INPUT];
	char* tok;
	int count = 0;
	if (input[0] == '/') {
		tok = strtok (input, "/");
		subdirectories[count] = tok;
		count++;
		while(tok) {
			tok = strtok(NULL, "/");
			subdirectories[count] = tok;
			count++;
		}
		count = count - 2;
		file_name = subdirectories[count];
	}
	
// testing prints
	//int i;
	//for (i = 0; i < count-1; i++) {
		//printf("%s\n", subdirectories[i]);
	//}
	//printf("%s\n", subdirectories[count]);	
	//printf("%s\n", file_name);	
	printf("count = %d\n", count);	
	printf("len = %zu\n",sizeof(subdirectories)/sizeof(subdirectories[0]));
	
	
	// set up file to copy into disk 
	int fd2 = open(file_name, O_RDWR);
	if (fd2 < 0) {
		printf("File not found.\n");
		munmap(memblock, buff.st_size);
		close(fd);
		exit(1);
	}
	struct stat buff2;
	fstat(fd2, &buff2);
	int file_size = buff2.st_size;
	char* inblock = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd2, 0);
	if (inblock == MAP_FAILED) {
		printf("Error: could not map input file memory.\n");
		munmap(memblock, buff.st_size);
		close(fd);
		close(fd2);
		exit(1);
	}
	
	// check if disk has room for input file
	int total_size = get_total_size(memblock);
	int free_size = get_free_size(memblock, total_size);
	
	if (free_size < file_size) {
		printf("Not enough free space in the disk image.\n");
		munmap(memblock, buff.st_size);
		munmap(inblock, file_size);
		close(fd);
		close(fd2);
		exit(1);
	}
	
	// search for location of subdirectory
	
	// copy file to location ***

	
	
	// clean up
	munmap(memblock, buff.st_size);
	munmap(inblock, file_size);
	close(fd);
	close(fd2);
	
	return 0;
}