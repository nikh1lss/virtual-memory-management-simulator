#include "translate.h"

/**
 * Parses a logical address to extract the page number and offset.
 *
 * @param logicalAddress The 32-bit logical address
 * @param pageNumber Pointer to store the extracted page number
 * @param offset Pointer to store the extracted offset
 */
void parseLogicalAddress(unsigned int logicalAddress, unsigned int *pageNumber,
                         unsigned int *offset) {
    // Consider only the rightmost 22 bits using bitwise & operator with
    // LOGICAL_ADDRESS_MASK
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
unsigned int calculatePhysicalAddress(unsigned int frameNumber,
                                      unsigned int offset) {
    return (frameNumber * FRAME_SIZE) + offset;
}
