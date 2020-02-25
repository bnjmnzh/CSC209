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

    for (i = 0; i < iterations; i++) {
        int n = fork();
        if (n < 0) {
            perror("fork");
            exit(1);
        } else if (n == 0) { // child process
	    exit(0);
	}
    }


    for (i = 0; i < iterations; i++) {
    	pid_t pid;
	int status;
	if( (pid = wait(&status)) == -1) {
	    perror("wait");
	} else {
	    printf("%d -> %d\n", getpid(), pid);
	}
    }

    return 0;
}
