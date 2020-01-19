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

int ipow(int base, int exp) {
	int result = 1;
	for(;;) {
	    if (exp & 1) {
		result *= base;
	    }
	    exp >>= 1;
	    if (!exp) break;
	    base *= base;
	}
	return result;
}

int get_ith_from_right(int num, int i) {
    num = num / ipow(BASE, i);
    return num % BASE;
}

int get_ith_from_left(int num, int i) {
    int j = count_digits(num) - i - 1;
    num = num / ipow(BASE, j);
    return num % BASE;
}

void add_to_tally(int num, int i, int *tally) {
    int digit = get_ith_from_left(num, i);
    tally[digit]++;
    return;
}
