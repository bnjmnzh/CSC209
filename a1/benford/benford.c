#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
void print_digits(int *digits);

int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }
    int position = strtol(argv[1], NULL, 10);
    int digits[BASE] = { 0 };
    int num_read;

    if (argc == 3) { // Input from a file
	FILE *input_set;
	input_set = fopen(argv[2], "r");
	if (input_set == NULL) {
	   fprintf(stderr, "Error opening file\n");
	   return 1;
	}

	while (fscanf(input_set, "%d", &num_read) == 1) {
	    add_to_tally(num_read, position, digits);
	}

	if (fclose(input_set) != 0) {
	   fprintf(stderr, "fclose failed\n");
	   return 1;
	}
    } else { // redirected input
	while (fscanf(stdin, "%d", &num_read) == 1) {
	    add_to_tally(num_read, position, digits);
	}
    }
    print_digits(digits);
    return 0;
 }

void print_digits(int *digits) {    
    for (int i = 0; i < BASE; i++) {
    	printf("%ds: %d\n", i, digits[i]);
    }
}
