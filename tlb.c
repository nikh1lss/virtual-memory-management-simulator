#include "translate.h"

/** Flushes a TLB entry for a given page number
 *
 * @param tlb The TLB array
 * @param pageNumber The page number to flush
 */
void flushTLBEntry(tlbEntry_t *tlb, unsigned int pageNumber) {
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
unsigned int countTLBEntries(tlbEntry_t *tlb) {
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
int searchTLB(tlbEntry_t *tlb, unsigned int pageNumber) {
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
void updateTLB(tlbEntry_t *tlb, unsigned int pageNumber,
               unsigned int frameNumber, unsigned int accessCount) {
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
