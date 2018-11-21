/**
   Georgia Ma
   V00849447
   CSC 360
   Assignment 3
**/

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/** Disk Parsing Functions **/

char* get_os_name(char* memblock) {
	char* name;
	int i;
	for (i = 0; i < 8; i++) {
		//strncpy(name[i], p[i+3], strlen(p[i+3])+1);
		name[i] = memblock[i+3];
	}
	return name;
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error: disk image needed as argument.\n");
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
		exit(1);
	}
	
	// parse disk data
	char* os_name = get_os_name(memblock);
/*
	char* disk_label = get_disk_label(memblock);
	int total_size = get_total_size(memblock);
	int free_size = get_free_size(memblock, total_size);
	
	int num_files = get_num_files(memblock);
	
	int num_fat_copies = get_num_fat_copies(memblock);
	int sectors_per_fat = get_sectors_per_fat(memblock);
*/	
	// print results
	printf("OS Name: %s\n", os_name);
/*	printf("Label of the disk: %s\n", disk_label);
	printf("Total size of the disk: %d bytes\n", total_size);
	printf("Free size of the disk: %d bytes\n\n", free_size);
	
	printf("==============\n");
	printf("The number of files in the root directory (including all files in the root directory and files in all subdirectories): %d\n\n", num_files);
	
	printf("=============\n");
	printf("Number of FAT copies: %d\n", num_fat_copies);
	printf("Sectors per FAT: %d\n\n", sectors_per_fat);
*/	
	// clean up
	munmap(memblock, buff.st_size);
	close(fd);

	return 0;
}