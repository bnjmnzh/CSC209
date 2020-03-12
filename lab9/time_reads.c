/* The purpose of this program is to practice writing signal handling
 * functions and observing the behaviour of signals.
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define max_int 100

/* Message to print in the signal handling function. */
#define MESSAGE "%ld reads were done in %ld seconds.\n"

/* Global variables to store number of read operations and seconds elapsed. 
 */
long num_reads, seconds;


void handler(int sig) {
    printf(MESSAGE, num_reads, seconds);
    exit(0);
}


/* The first command-line argument is the number of seconds to set a timer to run.
 * The second argument is the name of a binary file containing 100 ints.
 * Assume both of these arguments are correct.
 */

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: time_reads s filename\n");
        exit(1);
    }
    seconds = strtol(argv[1], NULL, 10);

    FILE *fp;
    if ((fp = fopen(argv[2], "r")) == NULL) {
      perror("fopen");
      exit(1);
    }

    struct sigaction newact;
    newact.sa_handler = handler;
    newact.sa_flags = 0;
    sigemptyset(&newact.sa_mask);
    if (sigaction(SIGPROF, &newact, NULL) == -1) {
        perror("sigaction");
	exit(1);
    }

    struct itimerval it_val;

    it_val.it_value.tv_sec = seconds;
    it_val.it_value.tv_usec = seconds / 1000000000;
    it_val.it_interval = it_val.it_value;

    if (setitimer(ITIMER_PROF, &it_val, NULL) == -1) {
        perror("setitimer");
	exit(1);
    }

    /* In an infinite loop, read an int from a random location in the file,
     * and print it to stderr.
     */
    for (;;) {
	int random_int = random() % max_int;
	if (fseek(fp, sizeof(int) * random_int, SEEK_SET) == -1) {
	    perror("fseek");
	    exit(1);
	}
	int read;
	if (fread(&read, sizeof(int), 1, fp) != 1) {
	    perror("read");
	    exit(1);
	}
	
	num_reads++;
	printf("%d\n", read);
    }
    return 1; // something is wrong if we ever get here!
}
