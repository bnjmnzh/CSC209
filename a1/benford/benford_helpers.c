#include <stdio.h>

#include "benford_helpers.h"

int count_digits(int num) {
    int i = num;
    int count = 0;
    while (i != 0) {
	count++;
	i = i / BASE;	
    }
    return count;
}

int get_ith_from_right(int num, int i) {
    // TODO: Implement.
    return 0;
}

int get_ith_from_left(int num, int i) {
    // TODO: Implement.
    return 0;
}

void add_to_tally(int num, int i, int *tally) {
    
    return;
}
