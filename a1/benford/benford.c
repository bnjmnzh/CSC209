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
    
    return 0;
 }

void print_digits(int *digits) {
    for (int i = 0; i < sizeof(digits) / sizeof(digits[0]); i++) {
    }
}
