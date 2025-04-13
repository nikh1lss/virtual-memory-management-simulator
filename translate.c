#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

/* Constants for 22-bit logical addresses */
#define LOGICAL_ADDRESS_BITS 22
#define PAGE_NUMBER_BITS 10
#define OFFSET_BITS 12
#define NUM_PAGES 1024
#define NUM_FRAMES 256
#define FRAME_SIZE 4096

/* Constants for masking */
#define LOGICAL_ADDRESS_MASK ((1u << LOGICAL_ADDRESS_BITS) - 1)
#define PAGE_NUMBER_MASK ((1u << PAGE_NUMBER_BITS) - 1)
#define OFFSET_MASK ((1u << OFFSET_BITS) - 1)


/* Page Table Entry struct with Present(1)/Absent(0) bit and Frame Number */
typedef struct { 
    unsigned int present;
    unsigned int frameNumber;
} pageTableEntry_t;

/* Function prototypes */
void parseLogicalAddress(unsigned int logicalAddress, unsigned int* pageNumber, unsigned int* offset);
unsigned int calculatePhysicalAddress(unsigned int frameNumber, unsigned int offset);
void task1(char* filename);
void task2(char* filename);

int main(int argc, char *argv[]) {
    char* filename = NULL;
    char* task = NULL;
    int opt;

    /* parse command-line arguments with -f -t options */
    while ((opt = getopt(argc, argv, "f:t:"))!= -1){
        switch (opt) {
            case 'f':
                filename = optarg;
                break;

            case 't': 
                task = optarg;
                break;
            
            case '?':
                fprintf(stderr, "Missing argument\n");
                exit(EXIT_FAILURE);
            
            default:
                abort();
        }
    }

    /* check if required arguments are provided */
    if (!filename || !task) {
        fprintf(stderr, "Missing required arguments\n");
        exit(EXIT_FAILURE);
    }

    /* run specific task given as argument */
    if (strcmp(task, "task1") == 0) {
		task1(filename);
	} else if (strcmp(task, "task2") == 0) {
		task2(filename);
	} else if (strcmp(task, "task3") == 0 || strcmp(task, "task4") == 0) {
		fprintf(stderr, "Task not implemented yet\n");
		exit(EXIT_FAILURE);
	}

    return 0;
}


/**
 * Parses a logical address to extract the page number and offset.
 * 
 * @param address The 32-bit logical address
 * @param page_number Pointer to store the extracted page number
 * @param offset Pointer to store the extracted offset
 */
 void parseLogicalAddress(unsigned int logicalAddress, unsigned int* pageNumber, unsigned int* offset) {
    /* Consider only the rightmost 22 bits using bitwise & operator with LOGICAL_ADDRESS_MASK */
    unsigned int maskedAddress = logicalAddress & LOGICAL_ADDRESS_MASK; // could also use 0x003FFFFF but less readable

    /* Extract the page number (bits 12 to 22 of the maskedAddress) */
    *pageNumber = (maskedAddress >> OFFSET_BITS) & PAGE_NUMBER_MASK;

    /* Extract the offset (last 12 bits of maskedAddress) */
    *offset = maskedAddress & OFFSET_MASK;

    return;
 }


/**
 * Executes Task 1: Parse logical addresses and print page number and offset
 * in required format: logical-address=<laddress>,page-number=<pnumber>,offset=<poffset>
 * 
 * @param filename The input file containing logical addresses
 */
void task1(char* filename) {
    unsigned int logicalAddress, pageNumber, offset;

    /* Open filename for reading */
    FILE* file = fopen(filename, "r");

    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    /* Process each logical address in file */
    while (fscanf(file, "%u", &logicalAddress) != EOF) {

        parseLogicalAddress(logicalAddress, &pageNumber, &offset);

        /* print result in required format */
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress, pageNumber, offset); 
    }

    fclose(file);

    return;
}

/**
 * Calculates the physical address based on frame number and offset.
 * 
 * @param frame_number The frame number
 * @param offset The offset within the frame
 * @return The physical address
 */
unsigned int calculatePhysicalAddress(unsigned int frameNumber, unsigned int offset) {
    return (frameNumber * FRAME_SIZE) + offset;
}


/**
 * Executes Task 2: Translate logical addresses to physical addresses using a page table
 * in required format: 
 * page-number=<pnumber>,page-fault=<pfault>,frame-number=<fnumber>,physical-address=<paddress>
 * 
 * @param filename The input file containing logical addresses
 */
void task2(char* filename) {
    unsigned int logicalAddress, pageNumber, offset;
    unsigned int nextFreeFrame = 0;

    /* open file for reading */
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    /* Initialize page table (all entries are not present therefore initialize all entries in struct to 0) */
    pageTableEntry_t pageTable[NUM_PAGES] = {0}; 

    /* Initialize free frame list to 0 (All frames are initially free) */
    unsigned int freeFrameList[NUM_FRAMES] = {0};

    /* process each logical address in the file */
    while (fscanf(file, "%u", &logicalAddress) != EOF) {
        parseLogicalAddress(logicalAddress, &pageNumber, &offset);

        /* print task 1 output */
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress, pageNumber, offset); 

        /* check if page is already allocated to a physical frame */
        unsigned int pageFault = !pageTable[pageNumber].present;

        /* handle page fault */
        if (pageFault) {
            /* allocate the page to the next lowest free frame */
            pageTable[pageNumber].present = 1;
			pageTable[pageNumber].frameNumber = nextFreeFrame;

            /* Mark the frame as used */
            freeFrameList[nextFreeFrame] = 1;

            /* Find the next free frame for future allocations 
             * (note that since we are never evicting pages in task 2
             *  this logic is not required, we could simply increment
             *  nextFreeFrame by 1, but in order to use the freeFrameList array
             *  we implement nextFreeFrame logic as so)
             */
            while (nextFreeFrame < NUM_PAGES && freeFrameList[nextFreeFrame]) {
                ++nextFreeFrame;
            }
        }

        /* Calculate the physical address */
        unsigned int physicalAddress = calculatePhysicalAddress(pageTable[pageNumber].frameNumber, offset);

        /* print task 2 output in required format */
        printf("page-number=%u,page-fault=%u,frame-number=%u,physical-address=%u\n", 
            pageNumber, pageFault, pageTable[pageNumber].frameNumber, physicalAddress);
    }

    fclose(file);

    return;
}