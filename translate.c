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
#define TLB_SIZE 32

/* Constants for masking */
#define LOGICAL_ADDRESS_MASK ((1u << LOGICAL_ADDRESS_BITS) - 1)
#define PAGE_NUMBER_MASK ((1u << PAGE_NUMBER_BITS) - 1)
#define OFFSET_MASK ((1u << OFFSET_BITS) - 1)


/* Page Table Entry struct with Present(1)/Absent(0) bit and Frame Number */
typedef struct { 
    unsigned int present;
    unsigned int frameNumber;
} pageTableEntry_t;

/* Frame table entry to support which page is mapped to each frame if present bit is 1, 
 * Otherwise, ignore page number 
 */
typedef struct {
    unsigned int present;
    unsigned int pageNumber;
} frameTableEntry_t;

/* TLB entry struct with page number, frame number, and a last used counter for LRU */
typedef struct {
    unsigned int pageNumber;
    unsigned int frameNumber;
    unsigned int present;
    unsigned int lastUsed;
} tlbEntry_t;

/* System structure to simulate an OS holding all memory tables and counters */
typedef struct {
    pageTableEntry_t pageTable[NUM_PAGES];
    frameTableEntry_t frameTable[NUM_FRAMES];
    tlbEntry_t tlb[TLB_SIZE];
    unsigned int nextFreeFrame;
    unsigned int oldestFrame;
    unsigned int accessCount;
} systemState_t;


/* Function prototypes */
void parseLogicalAddress(unsigned int logicalAddress, unsigned int* pageNumber, unsigned int* offset);
unsigned int calculatePhysicalAddress(unsigned int frameNumber, unsigned int offset);
int searchTLB(tlbEntry_t* tlb, unsigned int pageNumber);
void updateTLB(tlbEntry_t* tlb, unsigned int pageNumber, unsigned int frameNumber, unsigned int accessCount);
void flushTLBEntry(tlbEntry_t* tlb, unsigned int pageNumber);
unsigned int countTLBEntries(tlbEntry_t* tlb);
FILE* openInputFile(char* filename);
void initializeSystem(systemState_t* system);
void handlePageFault(systemState_t* system, unsigned int pageNumber, int taskLevel);
void processAddress(FILE* file, systemState_t* system, int taskLevel);
void runSimulation(char* filename, char* task);


int main(int argc, char *argv[]) {
    char* filename = NULL;
    char* task = NULL;
    int opt;

    // parse command-line arguments with -f -t options 
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

    // check if required arguments are provided 
    if (!filename || !task) {
        fprintf(stderr, "Missing required arguments\n");
        exit(EXIT_FAILURE);
    }

    runSimulation(filename, task);

    return 0; 
}


/**
 * Creates system and processes specific task
 * 
 * @param filename The input file containing logical addresses
 * @param taskLevel Which task to process
 */
void runSimulation(char* filename, char* task) {
    FILE* file = openInputFile(filename);
    systemState_t system;
    initializeSystem(&system);

    processAddress(file, &system, task[4] - '0');

    fclose(file);
}

/**
 * Opens the input file for processing
 * 
 * @param filename The input file containing logical addresses
 * @return File pointer to the opened file
 */
FILE* openInputFile(char* filename) {
    FILE* file = fopen(filename, "r");

    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    return file;
}


/** 
 * Initializes the state of the system/OS
 * 
 * @param system Pointer to a system struct that we are simulating into
 */
void initializeSystem(systemState_t* system) {
    // Initilize entries in struct to 0
    memset(system->pageTable, 0, sizeof(system->pageTable));
    memset(system->frameTable, 0, sizeof(system->frameTable));
    memset(system->tlb, 0, sizeof(system->tlb));

    system->accessCount = 0;
    system->nextFreeFrame = 0;
    system->oldestFrame = 0;
}

/**
 * Handles a page fault by either allocating a free frame or evicting a page,
 * Also handles the the case if a TLB is in use (Task 4)
 * 
 * @param system Pointer to the system state
 * @param pageNumber The page number that caused the fault
 */
void handlePageFault(systemState_t* system, unsigned int pageNumber, int taskLevel) {
    // Check if there are any free frames remaining
    if (system->nextFreeFrame == NUM_FRAMES) {
        // There are no free frames, need to evict with FIFO
        unsigned int evictedPage = system->frameTable[system->oldestFrame].pageNumber;

        // Evict the page
        system->pageTable[evictedPage].present = 0;

        // Allocate the new page to oldest frame
        system->pageTable[pageNumber].present = 1;
        system->pageTable[pageNumber].frameNumber = system->oldestFrame;
        system->frameTable[system->oldestFrame].pageNumber = pageNumber;

        printf("evicted-page=%u,freed-frame=%u\n", evictedPage, system->oldestFrame);

        // increment or wrap oldestFrame back to 0 after it reaches end of frame count
        system->oldestFrame = (system->oldestFrame + 1) % NUM_FRAMES;

        // Check if a TLB is in use and if evicted page is present for flushing
        if (taskLevel == 4) {
            flushTLBEntry(system->tlb, evictedPage);
        }

    } else {
        // There are free frames remaining (nextFreeFrame < NUM_FRAMES)
        system->pageTable[pageNumber].frameNumber = system->nextFreeFrame;
        system->pageTable[pageNumber].present = 1;
        system->frameTable[system->nextFreeFrame].pageNumber = pageNumber;
        system->frameTable[system->nextFreeFrame].present = 1;

        // Find next free frame
        while (system->nextFreeFrame < NUM_FRAMES && system->frameTable[system->nextFreeFrame].present) {
            ++system->nextFreeFrame;
        }
    }
}


/**
 * Processes a single logical address based on task level
 * 
 * @param file The input file
 * @param system Pointer to the system state
 * @param taskLevel The task level (1-4)
 */
void processAddress(FILE* file, systemState_t* system, int taskLevel) {
    unsigned int logicalAddress, pageNumber, offset;

    // Process each logical address in the file
    while (fscanf(file, "%u", &logicalAddress) != EOF) {
        ++system->accessCount;
        parseLogicalAddress(logicalAddress, &pageNumber, &offset);

        // Print Task 1 output for all tasks
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress, pageNumber, offset);

        // TLB handling for task 4
        if (taskLevel == 4) {
            int tlbIndex = searchTLB(system->tlb, pageNumber);
            unsigned int tlbHit = (tlbIndex != -1);

            if (tlbHit) {
                unsigned int frameNumber = system->tlb[tlbIndex].frameNumber;
                unsigned int physicalAddress = calculatePhysicalAddress(frameNumber, offset);
                system->tlb[tlbIndex].lastUsed = system->accessCount;

                printf("tlb-hit=%u,page-number=%u,frame=%u,physical-address=%u\n", 
                    tlbHit, pageNumber, frameNumber, physicalAddress);

            } else {
                // TLB miss
                printf("tlb-hit=%u,page-number=%u,frame=none,physical-address=none\n", 
                    tlbHit, pageNumber);
            }
        }

        // Page fault handling for tasks 2-4
        if (taskLevel >= 2) {
            unsigned int pageFault = !system->pageTable[pageNumber].present;

            if (pageFault) {
                handlePageFault(system, pageNumber, taskLevel);
            }

            // Update TLB for task 4
            if (taskLevel == 4) {
                updateTLB(system->tlb, pageNumber, system->pageTable[pageNumber].frameNumber, system->accessCount);
            }

            unsigned int physicalAddress =  calculatePhysicalAddress(system->pageTable[pageNumber].frameNumber, offset);

            // Print task2+ output
            printf("page-number=%u,page-fault=%u,frame-number=%u,physical-address=%u\n", 
                pageNumber, pageFault, system->pageTable[pageNumber].frameNumber, physicalAddress);
        }
    }
}


/**
 * Parses a logical address to extract the page number and offset.
 * 
 * @param logicalAddress The 32-bit logical address
 * @param pageNumber Pointer to store the extracted page number
 * @param offset Pointer to store the extracted offset
 */
void parseLogicalAddress(unsigned int logicalAddress, unsigned int* pageNumber, unsigned int* offset) {
    // onsider only the rightmost 22 bits using bitwise & operator with LOGICAL_ADDRESS_MASK 
    unsigned int maskedAddress = logicalAddress & LOGICAL_ADDRESS_MASK; 

    // Extract the page number (bits 12 to 22 of the maskedAddress) 
    *pageNumber = (maskedAddress >> OFFSET_BITS) & PAGE_NUMBER_MASK;

    // Extract the offset (last 12 bits of maskedAddress) 
    *offset = maskedAddress & OFFSET_MASK;

    return;
}


/**
 * Calculates the physical address based on frame number and offset.
 * 
 * @param frameNumber The frame number
 * @param offset The offset within the frame
 * @return The physical address
 */
unsigned int calculatePhysicalAddress(unsigned int frameNumber, unsigned int offset) {
    return (frameNumber * FRAME_SIZE) + offset;
}


/** Flushes a TLB entry for a given page number
 * 
 * @param tlb The TLB array
 * @param pageNumber The page number to flush
 */
void flushTLBEntry(tlbEntry_t* tlb, unsigned int pageNumber) {
    int index = searchTLB(tlb, pageNumber);

    if (index != -1) {
        tlb[index].present = 0;
        printf("tlb-flush=%u,tlb-size=%u\n", pageNumber, countTLBEntries(tlb));
    }

    // Page number not found in TLB, do nothing
    return;
}


/** 
  * Counts the number of present entries in TLB
  * 
  * @param tlb The TLB array
  * @return The number of present entries
  */
unsigned int countTLBEntries(tlbEntry_t* tlb) {
    int count = 0;

    for (int i = 0; i < TLB_SIZE; ++i) {
        count += tlb[i].present;
    }
    
    return count;
}


/**
 * Searchs the TLB for a page number
 * 
 * @param tlb The TLB array
 * @param pageNumber the page number to search for 
 * @return The index of the entry in the TLB, if not found return -1
 */
int searchTLB(tlbEntry_t* tlb, unsigned int pageNumber) {
    for (int i = 0; i < TLB_SIZE; ++i) {
        if (tlb[i].present && tlb[i].pageNumber == pageNumber) {
            return i;
        }
    }

    // not found
    return -1;
}


/**
 * Updates the TLB to include new mapping
 * 
 * @param tlb The TLB array
 * @param pageNumber The page number to add
 * @param frameNumber The frameNumber to add
 * @param accessCount Current access counter for LRU
 */
void updateTLB(tlbEntry_t* tlb, unsigned int pageNumber, unsigned int frameNumber, unsigned int accessCount) {
    int emptyIndex = -1;
    int lruIndex = 0;
    int tlbIndex = searchTLB(tlb, pageNumber);
    unsigned int tlbHit = (tlbIndex != -1);

    // Don't update TLB if tlbhit
    if (tlbHit) {
        return;
    }

    // look for an empty entry or determine the LRU entry
    for (int i = 0; i < TLB_SIZE; ++i) {
        if (!tlb[i].present) {
            emptyIndex = i;
            break;
        }

        if (tlb[i].lastUsed < tlb[lruIndex].lastUsed) {
            lruIndex = i;
        }
    }

    // If there is an empty entry, allocate new mapping to it
    if (emptyIndex != -1) {
        tlb[emptyIndex].present = 1;
        tlb[emptyIndex].pageNumber = pageNumber;
        tlb[emptyIndex].frameNumber = frameNumber;
        tlb[emptyIndex].lastUsed = accessCount;
        printf("tlb-remove=none,tlb-add=%u\n", pageNumber);
        return;
    }

    // Otherwise replace the LRU entry
    printf("tlb-remove=%u,tlb-add=%u\n", tlb[lruIndex].pageNumber, pageNumber);
    tlb[lruIndex].pageNumber = pageNumber;
    tlb[lruIndex].frameNumber = frameNumber;
    tlb[lruIndex].lastUsed = accessCount;

    return;
}