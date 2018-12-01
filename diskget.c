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
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define SECTOR_SIZE 512
#define MAX_INPUT 256

// subdirectory struct to keep track of names and locations
typedef struct subdirectory {
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
		byte1 = memblock[SECTOR_SIZE + (int)((3*i)/2)] & 0b11110000;
		byte2 = memblock[SECTOR_SIZE + (int)((3*i)/2)+1] & 0b11111111;
		entry = (byte1 >> 4) + (byte2 << 4);
	}
	return entry;
}

int find_file(char* memblock, int d, int sub, char* target) {
	int count = 0;
	struct subdirectory subdirectories[MAX_INPUT];
	
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

		// if a subdirectory is found, save it for later
		if ((temp & 0x10)){
			if (memblock[offset] != '.') {
				logical_cluster = (int)memblock[offset+26] + ((int)memblock[offset+27] << 8);
				if (logical_cluster != 0 && logical_cluster != 1) {
					subdirectories[count].location = logical_cluster;
					
					count++;
				}
			}
		} else {
			// otherwise, check if file name matches the target file
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

			strcat(file_name, ".");
			strcat(file_name, file_extension);
			
			// return location if file found
			if (!strcmp(target, file_name)) {
				return offset;
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

	// search through subdirectories	
	for (i = 0; i < count; i++) {
		int deeper = find_file(memblock, (31+subdirectories[i].location)*SECTOR_SIZE, 1, target);
		if (deeper > 0) {
			return deeper;
		}
	}
	
	return -1;
}

void copy_file(char* memblock, char* outblock, int location, int size) {
	int remaining = size;
	int logical_cluster = (int)memblock[location+26] + ((int)memblock[location+27] << 8);
	int p_a = (31+logical_cluster)*SECTOR_SIZE;
	int fat;
	
	int i;
	int offset;
L2_START:
	for (i = 0; i < SECTOR_SIZE; i++) {
		if (!remaining) {
			return;
		}
		offset = i+p_a;
		outblock[size - remaining] = memblock[offset];
		remaining--;
	}

	// check for another sector 
	int fat = get_fat(memblock, logical_cluster);
	if ((fat != 0x00) && ((fat < 0xFF0) || (fat > 0xFFF))) {
		p_a = (31+fat)*SECTOR_SIZE;
		logical_cluster = fat;
		goto L2_START;
	} 
	
	return; 
}


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
	
	// convert fime name to upper case
	char* name = argv[2];
	char* s = name;
	while (*s) {
		*s = toupper((unsigned char) *s);
		s++;
	}
	
	// find file location in disk
	int location = find_file(memblock, SECTOR_SIZE*19, 0, name);	
	
	// file not found
	if (location < 0) {
		printf("File not found.\n");
		
		// clean up and exit
		munmap(memblock, buff.st_size);
		close(fd);
		return 0;
	}
	
	
	
	printf("file is found!!\n");					// get rid of later plz 
	//printf("location: %d\n", location);
	
	
	
	// find size of file
	int file_size = (memblock[location+28] & 0xFF) + ((memblock[location+29] & 0xFF) << 8) + 
								((memblock[location+30] & 0xFF) << 16) + ((memblock[location+31] & 0xFF) << 24);

	//printf("%d\n", file_size);
	
	// set up output file to copy to
	int fd2 = open(argv[2], O_RDWR | O_CREAT, 0666);
	if (fd2 < 0) {
		printf("Error: could not open new file.\n");
		munmap(memblock, buff.st_size);
		close(fd);
		exit(1);
	}

	// write \0 at the last byte
	int output = lseek(fd2, file_size-1, SEEK_SET);
	if (output == -1) {
		munmap(memblock, buff.st_size);
		close(fd);
		close(fd2);
		printf("Error: could not seek to end of file.\n");
		exit(1);
	}
	output = write(fd2, "", 1);
	if (output != 1) {
		munmap(memblock, buff.st_size);
		close(fd);
		close(fd2);
		printf("Error: could not write last byte.\n");
		exit(1);
	}

	// map memory for output file
	char* outblock = mmap(NULL, file_size, PROT_WRITE, MAP_SHARED, fd2, 0);
	if (outblock == MAP_FAILED) {
		munmap(memblock, buff.st_size);
		munmap(outblock, file_size);
		close(fd);
		close(fd2);
		printf("Error: could not map output file memory.\n");
		exit(1);
	}

	// copy file over
	copy_file(memblock, outblock, location, file_size);
	
	// clean up
	munmap(memblock, buff.st_size);
	munmap(outblock, file_size);
	close(fd);
	close(fd2);
	
	return 0;

}