#include "translate.h"

/**
 * Initializes the state of the system/OS
 *
 * @param system Pointer to a system struct that we are simulating into
 */
void initializeSystem(systemState_t *system) {
    // Initilize entries in struct to 0
    memset(system->pageTable, 0, sizeof(system->pageTable));
    memset(system->frameTable, 0, sizeof(system->frameTable));
    memset(system->tlb, 0, sizeof(system->tlb));

    // Initialize counters
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
void handlePageFault(systemState_t *system, unsigned int pageNumber,
                     int taskLevel) {
    // Check if there are any free frames remaining
    if (system->nextFreeFrame == NUM_FRAMES) {
        // There are no free frames, need to evict with FIFO
        unsigned int evictedPage =
            system->frameTable[system->oldestFrame].pageNumber;

        // Evict the page
        system->pageTable[evictedPage].present = 0;

        // Allocate the new page to oldest frame
        system->pageTable[pageNumber].present = 1;
        system->pageTable[pageNumber].frameNumber = system->oldestFrame;
        system->frameTable[system->oldestFrame].pageNumber = pageNumber;

        printf("evicted-page=%u,freed-frame=%u\n", evictedPage,
               system->oldestFrame);

        // increment or wrap oldestFrame back to 0 after it reaches end of frame
        // count
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
        while (system->nextFreeFrame < NUM_FRAMES &&
               system->frameTable[system->nextFreeFrame].present) {
            ++system->nextFreeFrame;
        }
    }

    return;
}

/**
 * Processes a single logical address based on task level
 *
 * @param file The input file
 * @param system Pointer to the system state
 * @param taskLevel The task level (1-4)
 */
void processAddress(FILE *file, systemState_t *system, int taskLevel) {
    unsigned int logicalAddress, pageNumber, offset;

    // Process each logical address in the file
    while (fscanf(file, "%u", &logicalAddress) != EOF) {
        ++system->accessCount;
        parseLogicalAddress(logicalAddress, &pageNumber, &offset);

        // Print Task 1 output for all tasks
        printf("logical-address=%u,page-number=%u,offset=%u\n", logicalAddress,
               pageNumber, offset);

        // TLB handling for task 4
        if (taskLevel == 4) {
            int tlbIndex = searchTLB(system->tlb, pageNumber);
            unsigned int tlbHit = (tlbIndex != -1);

            if (tlbHit) {
                unsigned int frameNumber = system->tlb[tlbIndex].frameNumber;
                unsigned int physicalAddress =
                    calculatePhysicalAddress(frameNumber, offset);
                system->tlb[tlbIndex].lastUsed = system->accessCount;

                printf(
                    "tlb-hit=%u,page-number=%u,frame=%u,physical-address=%u\n",
                    tlbHit, pageNumber, frameNumber, physicalAddress);
                continue;

            } else {
                // TLB miss
                printf("tlb-hit=%u,page-number=%u,frame=none,physical-address="
                       "none\n",
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
                updateTLB(system->tlb, pageNumber,
                          system->pageTable[pageNumber].frameNumber,
                          system->accessCount);
            }

            unsigned int physicalAddress = calculatePhysicalAddress(
                system->pageTable[pageNumber].frameNumber, offset);

            // Print task2+ output
            printf("page-number=%u,page-fault=%u,frame-number=%u,physical-"
                   "address=%u\n",
                   pageNumber, pageFault,
                   system->pageTable[pageNumber].frameNumber, physicalAddress);
        }
    }

    return;
}
