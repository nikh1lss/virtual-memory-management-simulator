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


/* Function prototypes */
void parseLogicalAddress(unsigned int logicalAddress, unsigned int* pageNumber, unsigned int* offset);
unsigned int calculatePhysicalAddress(unsigned int frameNumber, unsigned int offset);
int searchTLB(tlbEntry_t* tlb, unsigned int pageNumber);
void updateTLB(tlbEntry_t* tlb, unsigned int pageNumber, unsigned int frameNumber, unsigned int accessCount);
void flushTLBEntry(tlbEntry_t* tlb, unsigned int pageNumber);
unsigned int countTLBEntries(tlbEntry_t* tlb);
void task1(char* filename);
void task2(char* filename);
void task3(char* filename);
void task4(char* filename);

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

    // run specific task given as argument 
    if (strcmp(task, "task1") == 0) {
        task1(filename);
    } else if (strcmp(task, "task2") == 0) {
        task2(filename);
    } else if (strcmp(task, "task3") == 0) {
        task3(filename);
    } else if (strcmp(task, "task4") == 0) {
        task4(filename);
    }

    return 0;
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
 * Executes Task 1: Parse logical addresses and print page number and offset
 * in required format: logical-address=<laddress>,page-number=<pnumber>,offset=<poffset>
 * 
 * @param filename The input file containing logical addresses
 */
void task1(char* filename) {
    unsigned int logicalAddress, pageNumber, offset;

    // Open filename for reading 
    FILE* file = fopen(filename, "r");

    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    // Process each logical address in file 
    while (fscanf(file, "%u", &logicalAddress) != EOF) {

        parseLogicalAddress(logicalAddress, &pageNumber, &offset);

        // print result in required format 
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress, pageNumber, offset); 
    }

    fclose(file);

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

    // open file for reading 
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    // Initialize page table (all entries are not present therefore initialize all entries in struct to 0) 
    pageTableEntry_t pageTable[NUM_PAGES] = {0}; 

    // Initialize free frame list to 0 (All frames are initially free and page number does not matter) 
    frameTableEntry_t frameTable[NUM_FRAMES] = {0};

    // process each logical address in the file
    while (fscanf(file, "%u", &logicalAddress) != EOF) {
        parseLogicalAddress(logicalAddress, &pageNumber, &offset);

        // print task 1 output
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress, pageNumber, offset); 

        // check if page is already allocated to a physical frame 
        unsigned int pageFault = !pageTable[pageNumber].present;

        if (pageFault) {
            // allocate the page to the next lowest free frame 
            pageTable[pageNumber].present = 1;
            pageTable[pageNumber].frameNumber = nextFreeFrame;

            // Mark the frame as used 
            frameTable[nextFreeFrame].present = 1;
            frameTable[nextFreeFrame].pageNumber = pageNumber;

            while (nextFreeFrame < NUM_PAGES && frameTable[nextFreeFrame].present) {
                ++nextFreeFrame;
            }
        }

        unsigned int physicalAddress = calculatePhysicalAddress(pageTable[pageNumber].frameNumber, offset);

        printf("page-number=%u,page-fault=%u,frame-number=%u,physical-address=%u\n", 
            pageNumber, pageFault, pageTable[pageNumber].frameNumber, physicalAddress);
    }

    fclose(file);

    return;
}


/** 
 * Executes Task 3: Page Replacement Algorithm (FIFO) 
 * in required format: 
 * evicted-page=<epage>,freed-frame=<fframe>
 *
 * @param filename The input file containing logical addresses 
 */
void task3(char* filename) {
    unsigned int logicalAddress, pageNumber, offset;
    unsigned int nextFreeFrame = 0;
    unsigned int oldestFrame = 0;

    // open file for reading
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    // Initialize page table (all entries are not present therefore initialize all entries in struct to 0) 
    pageTableEntry_t pageTable[NUM_PAGES] = {0}; 

    // Initialize free frame list to 0 (All frames are initially free and page number does not matter) 
    frameTableEntry_t frameTable[NUM_FRAMES] = {0};

    // process each logical address in the file
    while (fscanf(file, "%u", &logicalAddress) != EOF) {
        parseLogicalAddress(logicalAddress, &pageNumber, &offset);

        // print task 1 output
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress, pageNumber, offset);

        // check if page is already allocated to a physical frame 
        unsigned int pageFault = !pageTable[pageNumber].present;

        if (pageFault) {

            // check if there are any free frames remaining to allocate this page to
            if (nextFreeFrame == NUM_FRAMES) {

                // There are no free frames remaining
                unsigned int evictedPage = frameTable[oldestFrame].pageNumber;

                // evict the page
                pageTable[evictedPage].present = 0;

                // allocate new page
                pageTable[pageNumber].present = 1;
                pageTable[pageNumber].frameNumber = oldestFrame;
                frameTable[oldestFrame].pageNumber = pageNumber;

                /* print task3 output */
                printf("evicted-page=%u,freed-frame=%u\n", evictedPage, oldestFrame);

                // increment or wrap oldestFrame back to 0 after it reaches end of frame count
                oldestFrame = (oldestFrame + 1) % NUM_FRAMES;

            } else {

                // There are free frames remaining (nextFreeFrame < NUM_FRAMES)
                pageTable[pageNumber].present = 1;
                pageTable[pageNumber].frameNumber = nextFreeFrame;
                frameTable[nextFreeFrame].present = 1;
                frameTable[nextFreeFrame].pageNumber = pageNumber;

                while (nextFreeFrame < NUM_PAGES && frameTable[nextFreeFrame].present) {
                    ++nextFreeFrame;
                }
            }
        }

        unsigned int physicalAddress = calculatePhysicalAddress(pageTable[pageNumber].frameNumber, offset);

       /* print task 2 output in required format */
       printf("page-number=%u,page-fault=%u,frame-number=%u,physical-address=%u\n", 
        pageNumber, pageFault, pageTable[pageNumber].frameNumber, physicalAddress);
    }

    fclose(file);

    return;
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


/**
 * Executes Task 4: TLB Implementation
 * 
 * @param filename The input file containing logical addresses
 */
void task4(char* filename) {
    unsigned int logicalAddress, pageNumber, offset;
    unsigned int nextFreeFrame = 0;
    unsigned int oldestFrame = 0;
    unsigned int accessCount = 0;

    // open file for reading
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        exit(EXIT_FAILURE);
    }

    // Initialize page table (all entries are not present therefore initialize all entries in struct to 0) 
    pageTableEntry_t pageTable[NUM_PAGES] = {0}; 

    // Initialize free frame list to 0 (All frames are initially free and page number does not matter) 
    frameTableEntry_t frameTable[NUM_FRAMES] = {0};

    // Initialize TLB to 0
    tlbEntry_t tlb[TLB_SIZE] = {0};

    // Process each logical address in the file
    while (fscanf(file, "%u", &logicalAddress) != EOF) {
        ++accessCount;
        parseLogicalAddress(logicalAddress, &pageNumber, &offset);
 
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress, pageNumber, offset);

        // Check the TLB for page number mapping first
        int tlbIndex = searchTLB(tlb, pageNumber);
        unsigned int tlbHit = (tlbIndex != -1);

        if (tlbHit) {
            // Page number found in TLB
            unsigned int physicalAddress = calculatePhysicalAddress(tlb[tlbIndex].frameNumber, offset);
            tlb[tlbIndex].lastUsed = accessCount;

            // Print output for tlb hit
            printf("tlb-hit=%u,page-number=%u,frame=%u,physical-address=%u\n", 
                tlbHit, pageNumber, tlb[tlbIndex].frameNumber, physicalAddress);

        } else {
            // TLB miss
            printf("tlb-hit=%u,page-number=%u,frame=none,physical-address=none\n", 
                tlbHit, pageNumber);

            // Check if page is already allocated to a physical frame
            unsigned int pageFault = !pageTable[pageNumber].present;

            if (pageFault) {
                // Page is not in memory

                if (nextFreeFrame == NUM_FRAMES) {
                    // There are no free frames remaining
                    unsigned int evictedPage = frameTable[oldestFrame].pageNumber;

                    // Evict the page
                    pageTable[evictedPage].present = 0;

                    // Allocate a new page
                    pageTable[pageNumber].present = 1;
                    pageTable[pageNumber].frameNumber = oldestFrame;
                    frameTable[oldestFrame].pageNumber = pageNumber;

                    printf("evicted-page=%u,freed-frame=%u\n", evictedPage, oldestFrame);

                    // increment or wrap oldestFrame back to 0 after it reaches end of frame count
                    oldestFrame = (oldestFrame + 1) % NUM_FRAMES;


                    // If evicted page is present in TLB, flush it
                    flushTLBEntry(tlb, evictedPage);

                } else {
                    // There are free frames remaining (nextFreeFrame < NUM_FRAMES)
                    pageTable[pageNumber].present = 1;
                    pageTable[pageNumber].frameNumber = nextFreeFrame;
                    frameTable[nextFreeFrame].present = 1;
                    frameTable[nextFreeFrame].pageNumber = pageNumber;

                    while (nextFreeFrame < NUM_PAGES && frameTable[nextFreeFrame].present) {
                        ++nextFreeFrame;
                    }
                }
            }

            // Add the new mapping to the TLB
            updateTLB(tlb, pageNumber, pageTable[pageNumber].frameNumber, accessCount);

            unsigned int physicalAddress = calculatePhysicalAddress(pageTable[pageNumber].frameNumber, offset);

            /* print task 2 output in required format */
            printf("page-number=%u,page-fault=%u,frame-number=%u,physical-address=%u\n", 
                pageNumber, pageFault, pageTable[pageNumber].frameNumber, physicalAddress);

        }
    }

    fclose(file);

    return;
}

