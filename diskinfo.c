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

#define SECTOR_SIZE 512

/** Disk Parsing Functions **/

void name_and_label(char* memblock, char* name, char* label) {
	int i;
	for (i = 0; i < 8; i++) {
		name[i] = memblock[i+3];
		label[i] = memblock[i+43];
	}
	
	if (label[0] == ' ') {
		memblock += SECTOR_SIZE * 19;
		while (memblock[0]) {
			if (memblock[11] == 8) {
				for (i = 0; i < 8; i++) {
					label[i] = memblock[i];
				}
			}
			memblock += 32;
		}
	}
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
	for (i = 2; i < (size /SECTOR_SIZE); i++) {
		int entry = 0;
		int byte1 = 0;
		int byte2 = 0;
		
		// compute entry content
		if ((i % 2) == 0) {
			byte1 = memblock[SECTOR_SIZE + (int)((3*i)/2)+1] & 0x0F;
			byte2 = memblock[SECTOR_SIZE + (int)((3*i)/2)] & 0xFF;
			entry = (byte1 << 8) + byte2;
		} else {
			byte1 = memblock[SECTOR_SIZE + (int)((3*i)/2)] & 0xF0;
			byte2 = memblock[SECTOR_SIZE + (int)((3*i)/2)+1] & 0xFF;
			entry = (byte1 >> 4) + (byte2 << 4);
		}
		
		// if entry is 0x000, it is free
		if (!entry) {
			free_spaces++;
		}
	}
	int free_space = free_spaces * SECTOR_SIZE;
	return free_space;
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
	char* os_name = malloc(sizeof(char));	
	char* disk_label = malloc(sizeof(char));
	name_and_label(memblock, os_name, disk_label);
	
	int total_size = get_total_size(memblock);
	int free_size = get_free_size(memblock, total_size);
	
//	int num_files = get_num_files(memblock);
	
	int num_fat_copies = memblock[16];
	int sectors_per_fat = memblock[22] + (memblock[23] << 8);
	
	// print results
	printf("OS Name: %s\n", os_name);
	printf("Label of the disk: %s\n", disk_label);
	printf("Total size of the disk: %d bytes\n", total_size);
	printf("Free size of the disk: %d bytes\n\n", free_size);
	
	printf("==============\n");
//	printf("The number of files in the root directory (including all files in the root directory and files in all subdirectories): %d\n\n", num_files);
	
	printf("=============\n");
	printf("Number of FAT copies: %d\n", num_fat_copies);
	printf("Sectors per FAT: %d\n\n", sectors_per_fat);
	
	// clean up
	munmap(memblock, buff.st_size);
	free(os_name);
	free(disk_label);
	close(fd);

	return 0;
}