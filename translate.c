#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

/* Constants for 22-bit logical addresses */
#define LOGICAL_ADDRESS_BITS 22
#define PAGE_NUMBER_BITS 10
#define OFFSET_BITS 12

/* Constants for masking */
#define LOGICAL_ADDRESS_MASK ((1u << LOGICAL_ADDRESS_BITS) - 1)
#define PAGE_NUMBER_MASK ((1u << PAGE_NUMBER_BITS) - 1)
#define OFFSET_MASK ((1u << OFFSET_BITS) - 1)

/* Function prototypes */
void parseLogicalAddress(unsigned int logicalAddress, unsigned int* pageNumber, unsigned int* offset);
void task1(char* filename);


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
	} else {
		printf("Task not implemented yet\n");
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

