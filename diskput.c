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
#include <stdio.h>
#include <ctype.h>

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
		byte1 = memblock[SECTOR_SIZE + (int)((3*i)/2)] & 0b11110000;
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

int find_sub(char* memblock, int d, int sub, char** subs, int num_subs, int curr_target) {
	
	int i;
	int lim = SECTOR_SIZE*13;
	if (sub) {
		lim = SECTOR_SIZE;
	}
	int offset, logical_cluster;

// loop for traversing multiple sectors of a single subdirectory
L_START:	
	for (i = 0; i < lim; i = i+32) {
		offset = i+d;
		// skip free directory entries
		if (memblock[offset+0] == 0x00){
			break;
		} else if (memblock[offset+0] == 0xE5) {
			continue;
		}
		// temp variable to denote attribute (for some reason, I get a segmentation fault without it)
		char temp = memblock[offset+11];

		// if a subdirectory is found, check for the name
		if ((temp & 0x10)){
			char* file_name = malloc(sizeof(char));
			char* file_extension = malloc(sizeof(char));
			int j;
			for (j = 0; j < 8; j++) {
				if (memblock[offset+j] == ' ') {
					break;
				}
				file_name[j] = memblock[offset+j];
			}
			for (j = 0; j < 3; j++) {
				file_extension[j] = memblock[offset+j+8];
			}
			
			printf("%s\n", file_name);

			//strcat(file_name, ".");
			//strcat(file_name, file_extension);
			
			// if a matching name is found
			if (!strcmp(subs[curr_target], file_name)) {
				// go deeper if necessary
				if (curr_target < num_subs) {
					logical_cluster = (int)memblock[offset+26] + ((int)memblock[offset+27] << 8);
					if (logical_cluster != 0 && logical_cluster != 1) {
						int deeper = find_sub(memblock, (31+logical_cluster)*SECTOR_SIZE, 1, subs, num_subs, curr_target+1);
						if (deeper > 0) {
							return deeper;
						}
					}
				} else {
					return offset;
				}
			}
		}
	}
	
	if (sub) {
		// check for another sector if inside subdirectory
		int fat = get_fat(memblock, d);
		if ((fat != 0x00) && ((fat < 0xFF0) || (fat > 0xFFF))) {
			d = (31+fat)*SECTOR_SIZE;
			goto L_START;
		} 
	}
	
	return -1;
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
		while(tok) {
			subdirectories[count] = tok;
			count++;
			tok = strtok(NULL, "/");
		}
		file_name = subdirectories[count-1];
	}
	
	
	
// testing prints
/*	int i;
	for (i = 0; i < count; i++) {
		printf("%s\n", subdirectories[i]);
	}
	printf("%s\n", subdirectories[count-1]);	
	printf("%s\n", file_name);	
	printf("count = %d\n", count);	*/
	//printf("len = %zu\n",sizeof(subdirectories)/sizeof(subdirectories[0]));
	
	
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
	
	// convert names to upper case
	int i;
	for (i = 0; i < count; i++) {
		char* s = subdirectories[i];
		while (*s) {
			*s = toupper((unsigned char) *s);
			s++;
		}
		printf("%s\n", subdirectories[i]);
	}
	
	// search for location of subdirectory if required, otherwise file is copied to the root directory
	int location = SECTOR_SIZE*19;
	if (count) {
		location = find_sub(memblock, SECTOR_SIZE*19, 0, subdirectories, count-1, 0);
		if (location < 0) {
			printf("The directory not found.\n");
			munmap(memblock, buff.st_size);
			munmap(inblock, file_size);
			close(fd);
			close(fd2);
			exit(1);
		}
	} 
	
	printf("location: %d\n",location);
	
	
	
	
	// copy file to location ***

	
	
	// clean up
	munmap(memblock, buff.st_size);
	munmap(inblock, file_size);
	close(fd);
	close(fd2);
	
	return 0;
}