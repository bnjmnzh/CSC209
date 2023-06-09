#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
    int i;
    int iterations;

    if (argc != 2) {
        fprintf(stderr, "Usage: forkloop <iterations>\n");
        exit(1);
    }

    iterations = strtol(argv[1], NULL, 10);

    int n = 0;
    int parent = 0;

    for (i = 0; i < iterations; i++) {
	printf("ppid = %d, pid = %d, i = %d\n", getppid(), getpid(), i); 

	if (parent == 0) {
	    n = fork();
	}
	
	if (n > 0) {
	    parent = 1;
	}

        if (n < 0) {
            perror("fork");
            exit(1);
        }
	// printf("ppid = %d, pid = %d, i = %d\n", getppid(), getpid(), i);
    }

    if (i == iterations && n == 0) {
        exit(0);
    }

    pid_t pid;
    int status;
    if ( (pid = wait(&status)) == -1) {
        perror("wait");
	exit(1);
    }
    return 0;
}
