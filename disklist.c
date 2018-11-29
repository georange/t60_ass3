/**
   Georgia Ma
   V00849447
   CSC 360
   Assignment 3 part 2
**/

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define SECTOR_SIZE 512
#define MAX_INPUT 256

// subdirectory struct to keep track of names and locations
typedef struct subdirectory {
	char* name;
	int location;
	
} subdirectory;


/** Helper Methods **/

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

void get_name(char* memblock, int offset, char* file_name, char* file_extension) {
	int i;
	for (i = 0; i < 8; i++) {
		if (memblock[offset+i] == ' ') {
			break;
		}
		file_name[i] = memblock[offset+i];
	}
		
	for (i = 0; i < 3; i++) {
		file_extension[offset+i] = memblock[offset+i+8];
	}
		
	strcat(file_name, ".");
	strcat(file_name, file_extension);
}

void print_listings(char* memblock, int d, int sub, char* name) {
	int count = 0;
	struct subdirectory subdirectories[MAX_INPUT];
	
	// print header
	if(sub){
		printf("\n");
	}
	printf("%s\n", name);
	printf("==================\n");

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
		
		// check file type
		char file_type;
		if (temp & 0x10) {
			file_type = 'D';
		} else {
			file_type = 'F';
		}

		// find file name and extention
		char* file_name = malloc(sizeof(char));
		char* file_extension = malloc(sizeof(char));
		get_name(memblock, offset, file_name, file_extension);
		
		// save subdirectories for later
		if(file_type == 'D') {
			if (memblock[offset] != '.') {
				logical_cluster = (int)memblock[offset+26] + ((int)memblock[offset+27] << 8);
				if (logical_cluster != 0 && logical_cluster != 1) {
					subdirectories[count].name = file_name;
					subdirectories[count].location = logical_cluster;
					
					count++;
				}
			}
		} 	
		
		// get file size
		int file_size = (memblock[offset+28] & 0xFF) + ((memblock[offset+29] & 0xFF) << 8) + 
							((memblock[offset+30] & 0xFF) << 16) + ((memblock[offset+31] & 0xFF) << 24);
		// get times
		int year = (((memblock[offset+17] & 0b11111110)) >> 1) + 1980;
		int month = ((memblock[offset+16] & 0b11100000) >> 5) + (((memblock[offset+17] & 0b00000001)) << 3);
		int day = (memblock[offset+16] & 0b00011111);
			
		int h = (memblock[offset+15] & 0b11111000) >> 3;
		int min = ((memblock[offset+14] & 0b11100000) >> 5) + ((memblock[offset+15] & 0b00000111) << 3);
		
		// print results
		printf("%c %10d %20s %d-%d-%d %02d:%02d\n", file_type, file_size, file_name, year, month, day, h, min);	
		
	}
	
	if (sub) {
		// check for another sector if inside subdirectory
		int fat = get_fat(memblock, d);
		if ((fat != 0x00) && ((fat < 0xFF0) || (fat > 0xFFF))) {
			d = (31+fat)*SECTOR_SIZE;
			goto L_START;
		} 
	}
	
	// go through subdirectories
	for (i = 0; i < count; i++) {
		print_listings(memblock, (31+subdirectories[i].location)*SECTOR_SIZE, 1, subdirectories[i].name);
	}
		
	return;
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
		close(fd);
		exit(1);
	}
	
	print_listings(memblock, SECTOR_SIZE*19, 0, "ROOT_DIRECTORY");	
	
	// clean up
	munmap(memblock, buff.st_size);
	close(fd);
	
	return 0;
}