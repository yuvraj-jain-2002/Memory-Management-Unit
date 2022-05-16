/*
* @author: Yuvraj Jain, jainy3
* date: 25th March 2022
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>

// Defining all values
#define PAGE_SIZE 256
#define FRAME_SIZE 128
#define FRAMES 128
#define MEMORY FRAMES * PAGE_SIZE
#define PAGES 256
#define TLB_SIZE 16
#define OFFSET_BITS 8
#define OFFSET_MASK 255
#define BUFFER_SIZE 10

int virtual_address, offset, page_number, physical_address, frame_number, hit, tlbIndex;
int tlb_hits, addresses, page_faults;
int pageTable[PAGES];
char value;
char memory[MEMORY];
int mem_index = 0;
char * mmapfptr;
int Index;


/* Initializing a new data structure for the TLB
that stores the page and frame number.*/
struct TLBentry {
	int page_num;
	int frame_num;
};

struct TLBentry tlb[TLB_SIZE];

/* Setting all values of the TLB array to -1,
indicating that it is not in memory. */
void initialize_tlb(){

	//struct TLBentry tlb[TLB_SIZE];
	for (int i = 0; i < TLB_SIZE; i++) {
		tlb[i].page_num = -1;
		tlb[i].frame_num = -1;
	}
}

/* Searching the TLB to check whether the given page number
* exists in the TLB. If true, return 0, else return -1.
*/
int search_tlb(int p_num) {
	for (int i = 0; i < TLB_SIZE; i++) {
		if (tlb[i].page_num == p_num) {
			Index = i;
			return 0; //true
		}
	}
	return -1;
}

// Move the items of the entire array up by 1.
void update_tlb() {
	for (int i = 1; i < TLB_SIZE; i++) {
		tlb[i-1].page_num = tlb[i].page_num;
		tlb[i-1].frame_num = tlb[i].frame_num;
	}
}


int main(int argc, char const* argv[]) {
	char buffer[BUFFER_SIZE];

	FILE* file_pointer = fopen("addresses.txt", "r");
	FILE* BACKINGSTORE = fopen("BACKING_STORE.bin", "rb");
	int mmapfile_fd = open("BACKING_STORE.bin", O_RDONLY);
	mmapfptr = mmap(0, PAGES * PAGE_SIZE, PROT_READ, MAP_PRIVATE, mmapfile_fd, 0);

	/* Setting values of the page table to -1,
	* indicating that a page is not in memory.
	*/
	for (int i = 0; i < PAGES; i++) {
		pageTable[i] = -1;
	}

	initialize_tlb();

	// Loop until the end of the file is reached
	while (fgets(buffer, BUFFER_SIZE, file_pointer) != NULL) {

		addresses++; // Increment number of addresses by 1
		virtual_address = atoi(buffer);
		offset = virtual_address & OFFSET_MASK;
		page_number = virtual_address >> OFFSET_BITS;
		/* Check if the page number already exists in the page table
		If it does, get the corresponding values. */
		if (search_tlb(page_number) == 0) {
			tlb_hits++;
			frame_number = tlb[Index].frame_num;
			physical_address = (frame_number << OFFSET_BITS) | offset;
			value = memory[physical_address];
		}
		/* If it does not exist, check if the page number exists in the
		page table */
		else if (pageTable[page_number] != -1) {
			frame_number = pageTable[page_number];
			physical_address = (frame_number << OFFSET_BITS) | offset;
			value = memory[physical_address];
		}
		// Page fault
		else {
			// Copy page number from BACKING_STORE to memory
			memcpy(memory + mem_index * PAGE_SIZE, mmapfptr + page_number * PAGE_SIZE, PAGE_SIZE);
			int i = 0;
			// Set a free frame
			do {
				if (pageTable[i] == mem_index){
					pageTable[i] = -1;
				}
				i++;
			} while (i < PAGES);

			// Update page table by setting its value to the index of the available frame
			pageTable[page_number] = mem_index;

			// Increment page faults by 1
			page_faults++;
			mem_index = (mem_index + 1) % FRAME_SIZE;
			frame_number = pageTable[page_number];
			physical_address = (frame_number << OFFSET_BITS) | offset;
			value = memory[physical_address];
			/* Looping over the TLB table to check for an available space to insert the page
                           number and frame number.
			*/
			for (int i = 0; i <= TLB_SIZE; i++){
                                if (i == 15){ // Chekcing if the table is full
                                        update_tlb(); // FIFO removal
					// Inserts the new value at the bottom of the TLB table
                                        tlb[15].page_num = page_number;
                                        tlb[15].frame_num = frame_number;
                                        break;
                                }
				// Inerts the new value at the next available space
                                if (tlb[i].page_num == -1 && tlb[i].frame_num == -1){
                                        tlb[i].page_num = page_number;
                                        tlb[i].frame_num = frame_number;
                                }
                        }
		}
		printf("Virtual address: %d, Physical address: %d, Value: %d\n", virtual_address, physical_address, value);
	}
	printf("Total addresses = %d\n", addresses);
	printf("Page Faults = %d\n", page_faults);
	printf("TLB Hits = %d\n", tlb_hits);
	fclose(file_pointer);
	munmap(mmapfptr, PAGES * PAGE_SIZE);
	return 0;
}
