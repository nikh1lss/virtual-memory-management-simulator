#pragma once

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/* Frame table entry to support which page is mapped to each frame if present
 * bit is 1, Otherwise, ignore page number
 */
typedef struct {
    unsigned int present;
    unsigned int pageNumber;
} frameTableEntry_t;

/* TLB entry struct with page number, frame number, and a last used counter for
 * LRU */
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
void parseLogicalAddress(unsigned int logicalAddress, unsigned int *pageNumber,
                         unsigned int *offset);
unsigned int calculatePhysicalAddress(unsigned int frameNumber,
                                      unsigned int offset);
int searchTLB(tlbEntry_t *tlb, unsigned int pageNumber);
void updateTLB(tlbEntry_t *tlb, unsigned int pageNumber,
               unsigned int frameNumber, unsigned int accessCount);
void flushTLBEntry(tlbEntry_t *tlb, unsigned int pageNumber);
unsigned int countTLBEntries(tlbEntry_t *tlb);
void initializeSystem(systemState_t *system);
void handlePageFault(systemState_t *system, unsigned int pageNumber,
                     int taskLevel);
void processAddress(FILE *file, systemState_t *system, int taskLevel);
