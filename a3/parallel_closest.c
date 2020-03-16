#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"


/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 *
 * p: Pointer to an array of points sorted accordint to x coordinate
 * n: Number of points in array
 * pdmax: Maximum depth of the worker process tree rooted at the current process
 *        For the first call of this function, pdmax is the maximum depth of the 
 *        process tree as specified by the argument of the command line option -d
 * pcount: To be populated with the number of worker processes. Initiazlied to 0 
 *         at the start of the program. Parent process is not a worker progress.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {
    if (n < 4 || pdmax == 0) {
	// Maximum depth has been reached or <= 3 points is covered by closest_serial
        return closest_serial(p, n); 
    } else {
    	int mid = n / 2;
	struct Point mid_point = p[mid];

	int left_fd[2];
	if (pipe(left_fd) == -1) {
	    perror("pipe");
	    exit(1);
	}

	int left = fork();
	if (left < 0) {
	    perror("fork");
	    exit(1);
	} else if (left == 0) {
	    // Close read end in child pipe since child only writes
	    if (close(left_fd[0]) == -1) {
	        perror("close");
		exit(1);
	    }

	    double left_closest = closest_parallel(p, mid, pdmax - 1, pcount);
	    if (write(left_fd[1], &left_closest, sizeof(double)) != sizeof(double)) {
	        perror("write from left child to pipe");
		exit(1);
	    }

	    if (close(left_fd[1]) == -1) {
	        perror("close");
		exit(1);
	    }
	    exit(*pcount);
	} else {
	    // Close write end of pipe since parent only reads
	    if (close(left_fd[1]) == -1) {
	        perror("close");
		exit(1);
	    }
	}

	int right_fd[2];
	if (pipe(right_fd) == -1) {
	    perror("pipe");
	    exit(1);
	}

	int right = fork();
	if (right < 0) {
	    perror("fork");
	    exit(1);
	} else if (right == 0) {
	    // Close read end of pipe since child only writes
	    if (close(right_fd[0]) == -1) {
	        perror("close");
	        exit(1);
	    }
	    
	    double right_closest = closest_parallel(p + mid, n - mid, pdmax - 1, pcount);
	    if (write(right_fd[1], &right_closest, sizeof(double)) != sizeof(double)) {
		perror("write from right child to pipe");
		exit(1);
	    }
	    

	    if (close(right_fd[1]) == -1) {
	        perror("close");
		exit(1);
	    }
	    exit(*pcount);
	} else {
	    // Close write end of pipe since parent only reads
	    if (close(right_fd[1]) == -1) {
	        perror("close");
		exit(1);
	    }
	}	

	// Wait for both child processes to terminate
	int status_left, status_right;
	if ((wait(&status_left) == -1) || (wait(&status_right) == -1)) {
	    perror("wait");
	    exit(1);
	}

	if ((WIFEXITED(status_left) && (WIFEXITED(status_right)))) {
	    if ((WEXITSTATUS(status_left) == 1) || (WEXITSTATUS(status_right) == 1)) {
		// If any child exits with status 1 then parent also exits with status 1
	        fprintf(stderr, "Child terminated with exit status 1");
		exit(1);
	    } else {
	        (*pcount) += WEXITSTATUS(status_left) + WEXITSTATUS(status_right) + 2;
	    }
	}	

	double min_left, min_right;
	if (read(left_fd[0], &min_left, sizeof(double)) != sizeof(double)) {
	    perror("reading from pipe of left child");
	    exit(1);
	}	    
	if (read(right_fd[0], &min_right, sizeof(double)) != sizeof(double)) {
	    perror("reading from pipe of right child");
	    exit(1);
	}

	if ((close(left_fd[0]) == -1) || (close(right_fd[0]) == -1)) {
	    perror("close");
	    exit(1);
	}

	double d = min(min_left, min_right);

	// Build an array strip[] that contains points close 
	// (closer than d) to the line passing through the middle point
	struct Point *strip = malloc(sizeof(struct Point) * n);
	if (strip == NULL) {
	    perror("malloc");
	    exit(1);
	}
	int j = 0;
	for (int i = 0; i < n; i++ ) {
	    if (abs(p[i].x - mid_point.x) < d) {
	        strip[j] = p[i];
		j++;
	    }
	}

	double new_min = min(d, strip_closest(strip, j, d));
	free(strip);
	return new_min;	
    }
}

