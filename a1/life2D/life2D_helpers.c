#include <stdio.h>
#include <stdlib.h>

void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

void update_state(int *board, int num_rows, int num_cols) {
    int total = num_rows * num_cols;
    int new_board[total];
    for (int i = 0; i < total; i++) {
	// Edge cells don't change
    	if ((0 <= i && i <= num_cols - 1) || (total - num_cols <= i && i <= total - 1) || (i % num_cols == 0)
		||  ((i + 1) % num_cols == 0)) {
	    ;
	}
	else {
	    // west neighbour: i - 1
	    // east neightbour: i + 1
	    // north neighbour: i - num_col
	    // south neighbour: i + num_col
	    // north-east neighbour: i - num_col + 1
	    // north-west neighbour: i - num_col - 1
	    // south-east neighbour: i + num_col + 1
	    // south-west neighbour: i + num_col - 1
	    int count_alive = 0;
	    int neighbours[8] = {i - 1, i + 1, i - num_cols, i + num_cols, i - num_cols + 1, i - num_cols - 1,
		    i + num_cols + 1, i + num_cols - 1};
	    for (int j = 0; j < 8; j++) {
	    	if (board[neighbours[j]] == 1) { 
		    count_alive++;
		}	
	    }
	    if (board[i] == 1) {
		if (count_alive < 2 || count_alive > 3) {
		    new_board[i] = 0;
		} else {
		    new_board[i] = 1;
		}
	    } else if (board[i] == 0) {
		if (count_alive == 2 || count_alive == 3) {
		    new_board[i] = 1;
		} else {
		    new_board[i] = 0;
		}
	    }
	}
    }
    for (int i = 0; i < total; i++) {
	if (!((0 <= i && i <= num_cols - 1) || (total - num_cols <= i && i <= total - 1) || (i % num_cols == 0)
        	||  ((i + 1) % num_cols == 0))) {
    	   board[i] = new_board[i];
	}
    }
    return;
}
