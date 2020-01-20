#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }

    int num_rows = strtol(argv[1], NULL, 10);
    int num_col = strtol(argv[2], NULL, 10);
    int states = strtol(argv[3], NULL, 10);
    int board[num_rows * num_col];
    
    int i = 0;
    while(fscanf(stdin, "%d", &board[i]) == 1) {
	    i++;
    }

    for (int i = 0; i < states; i++) {
        print_state(board, num_rows, num_col);	    
    	update_state(board, num_rows, num_col);
    }

    return 0;
}
