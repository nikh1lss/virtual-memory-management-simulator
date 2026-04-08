#include "translate.h"

/* Function prototypes */
FILE *openInputFile(char *filename);
void runSimulation(char *filename, char *task);

int main(int argc, char *argv[]) {
    char *filename = NULL;
    char *task = NULL;
    int opt;

    // parse command-line arguments with -f -t options
    while ((opt = getopt(argc, argv, "f:t:")) != -1) {
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
void runSimulation(char *filename, char *task) {
    FILE *file = openInputFile(filename);
    int taskLevel = task[4] - '0';
    systemState_t system;

    initializeSystem(&system);
    processAddress(file, &system, taskLevel);
    fclose(file);

    return;
}

/**
 * Opens the input file for processing
 *
 * @param filename The input file containing logical addresses
 * @return File pointer to the opened file
 */
FILE *openInputFile(char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    return file;
}
