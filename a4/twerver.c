#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include "socket.h"

#ifndef PORT
    #define PORT 25475
#endif

#define LISTEN_SIZE 5
#define WELCOME_MSG "Welcome to CSC209 Twitter! Enter your username: \r\n"
#define SEND_MSG "send"
#define SHOW_MSG "show"
#define FOLLOW_MSG "follow"
#define UNFOLLOW_MSG "unfollow"
#define BUF_SIZE 256
#define MSG_LIMIT 8
#define FOLLOW_LIMIT 5
#define INVALID_USERNAME "Your username can't be blank or an existing username!\r\n"
#define INVALID "Invalid command!\r\n"
#define NOT_EXIST "User does not exist!\r\n"

struct client {
    int fd;
    struct in_addr ipaddr;
    char username[BUF_SIZE];
    char message[MSG_LIMIT][BUF_SIZE];
    struct client *following[FOLLOW_LIMIT]; // Clients this user is following
    struct client *followers[FOLLOW_LIMIT]; // Clients who follow this user
    char inbuf[BUF_SIZE]; // Used to hold input from the client
    char *in_ptr; // A pointer into inbuf to help with partial reads
    struct client *next;
};


// Provided functions. 
void add_client(struct client **clients, int fd, struct in_addr addr);
void remove_client(struct client **clients, int fd);

// These are some of the function prototypes that we used in our solution 
// You are not required to write functions that match these prototypes, but
// you may find them helpful when thinking about operations in your program.

// Send the message in s to all clients in active_clients (sequential list) 
void announce(struct client **clients_list, struct client *active_clients[], char *s);

// Send the message in s to all clients in active_clients (linked list)
void make_announcement(struct client **active_clients, char *s);

// Move client c from new_clients list to active_clients list. 
void activate_client(struct client *c, 
    struct client **active_clients_ptr, struct client **new_clients_ptr);

// Find a network newline in the first n characters of buf
int find_network_newline(const char *buf, int n);

// Read messages from a client, up till the network newline. 
// Return a string to the message, or NULL in case of any failure.
char * read_from_client(struct client **clients_list, struct client *client_ptr);

// Check if a username is valid
int validate_username(char *input, struct client *active_clients);

// Check if input is a send, show, follow, unfollow, quit command
int verify_input(char *input);

// Add client to to_follow's follwer list and add to_follow to client's following list
void follow(struct client **clients_list, struct client *client, struct client *to_follow);

// Remove client from to_follow's follower list and remove to_unfollow from client's following list.
void unfollow(struct client **clients_list, struct client *client, struct client *to_unfollow);

// Show messages previously sent by users this user is following
void show_messages(struct client **clients_list, struct client *client);

// Send message in message to all of client's followers
// Also store message in client's message history
void send_message(struct client **clients_list, struct client *client, char *message);

// Return a pointer to the active user with corresponding username
struct client * find_user(struct client *active_clients, char *username);

// The set of socket descriptors for select to monitor.
// This is a global variable because we need to remove socket descriptors
// from allset when a write to a socket fails. 
fd_set allset;

/* 
 * Create a new client, initialize it, and add it to the head of the linked
 * list.
 */
void add_client(struct client **clients, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));
    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));
    p->fd = fd;
    p->ipaddr = addr;
    p->username[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *clients;

    // initialize messages to empty strings
    for (int i = 0; i < MSG_LIMIT; i++) {
        p->message[i][0] = '\0';
    }

    // Initialize followers and following to NULL
    for (int i = 0; i < FOLLOW_LIMIT; i++) {
        p->followers[i] = NULL;
	p->following[i] = NULL;
    }

    *clients = p;
}

/* 
 * Remove client from the linked list and close its socket.
 * Also, remove socket descriptor from allset.
 */
void remove_client(struct client **clients, int fd) {
    struct client *curr = *clients;
    struct client *prev = NULL;
    // Find the previous node in the clients list to remove the node we want
    for (; curr != NULL && curr->fd != fd; curr = curr->next) {
        prev = curr;
    }

    if (curr != NULL) {
	struct client *temp;
	if (prev != NULL) { // curr is not the start of the list
	     temp = prev->next;
	     prev->next = temp->next;
	} else { // Prev is null so curr is the start of the list
	     *clients = curr->next;
	     temp = curr;
	}
	// Remove the client from other client's follower/following list
	for (int i = 0; i < FOLLOW_LIMIT; i++) {
	    if (temp->following[i] != NULL) {
	         for (int j = 0; j < FOLLOW_LIMIT; j++) {
		      if (temp->following[i]->followers[j] != NULL && temp->following[i]->followers[j]->fd == fd) {
		           temp->following[i]->followers[j] = NULL;
		      }
		 }	 
	    }
	}
	for (int i = 0; i < FOLLOW_LIMIT; i++) {
	     if (temp->followers[i] != NULL) {
	          for (int j = 0; j < FOLLOW_LIMIT; j++) {
		      if (temp->followers[i]->following[i] != NULL && temp->followers[i]->following[j]->fd == fd) {
		          temp->followers[i]->following[j] = NULL;
		      }
		  }
	     }
	}
	char announcement[12 + strlen(temp->username)];
	if (sprintf(announcement, "Good bye %s\r\n", temp->username) < 0) {
	    fprintf(stderr, "fprintf error\n");
	}
	printf("Good bye %s\n", temp->username);
	make_announcement(clients, announcement);
	printf("Removing client %d %s\n", fd, inet_ntoa(temp->ipaddr));
	FD_CLR(fd, &allset);
	close(temp->fd);
	free(temp);
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n", fd);
    }
}

// Send the message in s to all clients in active_clients.
// Assuming S is already terminated by \r\n
// This function takes a list in as argument
void announce(struct client **clients_list, struct client *active_clients[], char *s) {
    for (int i = 0; i < FOLLOW_LIMIT; i++) {
        if (active_clients[i] != NULL) {
	    if (write(active_clients[i]->fd, s, strlen(s)) == -1) {
	        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(active_clients[i]->ipaddr));
		remove_client(clients_list, active_clients[i]->fd);
	    }
	}
    }
}


// Send message in s to all clients in active_clients 
// Assuming s is already terminated by \r\n
// This function takes a linked list in as argument
void make_announcement(struct client **active_clients, char *s) {
    struct client *curr;
    for (curr = *active_clients; curr; curr = curr->next) {
        if (write(curr->fd, s, strlen(s)) == -1) {
	   fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(curr->ipaddr));
	   remove_client(active_clients, curr->fd);
	}
    }
}

// Move client c from new_clients list to active_clients list
void activate_client(struct client *c,
	struct client **active_clients_ptr, struct client **new_clients_ptr) {

    struct client *curr = *new_clients_ptr;
    struct client *prev = NULL;
    // Find the previous node in the new_clients list to remove the node
    for (; curr != NULL && curr->fd != c->fd; curr = curr->next) {
	prev = curr;
    }
    // p points to front of the linked list or the previous node of the one 
    if (curr != NULL) {
        if (prev != NULL) { // Curr is not the start of the list
	    struct client *temp;
	    temp = prev->next;
	    prev->next = temp->next;
	    temp->next = *active_clients_ptr;
	    *active_clients_ptr = temp;
	} else { // Curr is the start of the list
	    *new_clients_ptr = curr->next;
	    curr->next = *active_clients_ptr;
	    *active_clients_ptr = curr;
	}	    
	printf("%s has joined. \n", (*active_clients_ptr)->username);
    } else {
	    fprintf(stderr, "Trying to find fd %d, but I can't find it\n", c->fd);
    }
}


int main (int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;

    // If the server writes to a socket that has been closed, the SIGPIPE
    // signal is sent and the process is terminated. To prevent the server
    // from terminating, ignore the SIGPIPE signal. 
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // A list of active clients (who have already entered their names). 
    struct client *active_clients = NULL;

    // A list of clients who have not yet entered their names. This list is
    // kept separate from the list of active clients, because until a client
    // has entered their name, they should not issue commands or 
    // or receive announcements. 
    struct client *new_clients = NULL;

    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, LISTEN_SIZE);
    free(server); // this needs to be added to resolve valgrind error

    // Initialize allset and add listenfd to the set of file descriptors
    // passed into select 
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;

        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            exit(1);
        } else if (nready == 0) {
            continue;
        }

        // check if a new client is connecting
        if (FD_ISSET(listenfd, &rset)) {
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd, &q);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_client(&new_clients, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if (write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, 
                    "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_client(&new_clients, clientfd);
            }
        }

        // Check which other socket descriptors have something ready to read.
        // The reason we iterate over the rset descriptors at the top level and
        // search through the two lists of clients each time is that it is
        // possible that a client will be removed in the middle of one of the
        // operations. This is also why we call break after handling the input.
        // If a client has been removed, the loop variables may no longer be 
        // valid.
        int cur_fd, handled;
        for (cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if (FD_ISSET(cur_fd, &rset)) {
                handled = 0;

                // Check if any new clients are entering their names
                for (p = new_clients; p != NULL; p = p->next) {
                    if (cur_fd == p->fd) {
                        // Handle input from a new client who has not yet
                        // entered an acceptable name
			if (strlen(p->username) == 0) {
			    char *input;
			    if ( (input = read_from_client(&new_clients, p)) != NULL) {
                                // Check if input is one of send, follow, unfollow, show, quit
				int verify = verify_input(input);
			        if (verify == 0) {
				    // Client wants to quit
				    free(input);
				    remove_client(&new_clients, p->fd);
				    break;
				    
				} else if (verify != -1) {
				    free(input);

				    char *invalid = INVALID;
				    if (write(p->fd, invalid, strlen(invalid)) == -1) {
				        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(p->ipaddr));
					remove_client(&new_clients, p->fd);
					continue;
				    }
				    char *greeting = WELCOME_MSG;
				    if (write(p->fd, greeting, strlen(greeting)) == -1) {
				        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(p->ipaddr));
				        remove_client(&new_clients, p->fd);
					continue;
				    }
				    continue;
				}	

			        if (validate_username(input, active_clients) == 0) {
			            strncpy(p->username, input, (int)strlen(input));
				    free(input);

				    char joined[BUF_SIZE + 15];
				    if (sprintf(joined, "%s has joined. \r\n", p->username) < 0) {
				        fprintf(stderr, "sprintf error\n");
				    }
				    make_announcement(&active_clients, joined);
			            activate_client(p, &active_clients, &new_clients);
				    handled = 1;
			            break;
			        } else {
				    free(input);

			            char *error_message = INVALID_USERNAME;
                                    if (write(p->fd, error_message, strlen(error_message)) == -1) {
                                        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(p->ipaddr));
			                remove_client(&new_clients, p->fd);
					continue;
				    }
                                    char *greeting = WELCOME_MSG;
                                    if (write(p->fd, greeting, strlen(greeting)) == -1) {
                                        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(p->ipaddr));
                                        remove_client(&new_clients, p->fd);
					continue;
				    }
			        }
                            }
                        }
		    }
		}	    

                if (!handled) {
                    // Check if this socket descriptor is an active client
                    for (p = active_clients; p != NULL; p = p->next) {
                        if (cur_fd == p->fd) {
			    char *input;
                            if ( (input = read_from_client(&active_clients, p)) != NULL) {
			    	int verify = verify_input(input);
				printf("%s: %s\n", p->username, input);
				if (verify == -1) {
				    free(input);
				    char *error = INVALID;
				    if (write(p->fd, error, strlen(error)) == -1) {
				        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(p->ipaddr));
				        remove_client(&active_clients, p->fd);
					continue;
				    }
				    break;
				} else if (verify == 0) {
				    free(input);
				    remove_client(&active_clients, p->fd);
				    // Client wants to quit
				    break;

				} else if (verify == 2) {
				    free(input);
				    show_messages(&active_clients, p);
				    break;
				}

				// Find rest of the command
				char *rest = strchr(input, ' ') + 1;
				switch (verify) {
				    case 1: ;// send
					send_message(&active_clients, p, rest);
					free(input);
					break;

				    case 3: ;// follow
					struct client *to_follow = find_user(active_clients, rest);
					if (to_follow == NULL) {
					    char *error = NOT_EXIST; 
					    if(write(p->fd, error, strlen(error)) == -1) {
					        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(p->ipaddr));
					        remove_client(&active_clients, p->fd);
						continue;
					    }
					} else {
					    follow(&active_clients, p, to_follow);
					}
					free(input);
					break;

				    case 4: ;// unfollow
					struct client *to_unfollow = find_user(active_clients, rest);
					if (to_unfollow == NULL) {
					    char *error = NOT_EXIST;
					    if (write(p->fd, error, strlen(error)) == -1) {
					        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(p->ipaddr));
					        remove_client(&new_clients, p->fd);
						continue;
					    }
					} else {
					    unfollow(&active_clients, p, to_unfollow);
					}
					free(input);
					break;

				    default:
					printf("Something is wrong and you are probably wondering what. Well I don't know either\n");
				}
			    }
                        }
                    }
                }
            }
        }
    }	
    return 0;
}

/*
 * Search the first n characters of buf for a network newline (\r\n)
 * return one plus the index of the '\n' of the first network newline
 * or -1 if no network newline is found
 * 
 */
int find_network_newline(const char *buf, int n) {
    for (int i = 0; i < n - 1; i++) {
        if ((buf[i] == '\r') && (buf[i + 1] == '\n')) {
	        return i + 2;
	    }
    }
    return -1;
}

// Read from the client, handling partial reads with network newlines
// If a message is successfully read, it is placed in an available slot in clients messages
// The string is returned, and NULL is returned in any other case
char * read_from_client(struct client **clients_list, struct client *client_ptr) {
    int inbuf = 0;
    int room = sizeof(client_ptr->inbuf);
    client_ptr->in_ptr = client_ptr->inbuf;
    int nbytes = 0;
    char *message = NULL;
 
    if ((nbytes = read(client_ptr->fd, client_ptr->in_ptr, room)) == -1) {
        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(client_ptr->ipaddr));
	remove_client(clients_list, client_ptr->fd);
	return message;
    } else if (nbytes > 0) {
    // while ((nbytes = read(client_ptr->fd, client_ptr->in_ptr, room)) > 0) {
        printf("[%d] Read %d bytes\n", client_ptr->fd, nbytes);
        inbuf += nbytes;
        int where;
        int found = 0;

        while ((where = find_network_newline(client_ptr->inbuf, inbuf)) > 0) {
            client_ptr->inbuf[where - 2] = '\0';
            printf("[%d] Found network newline %s\n", client_ptr->fd, client_ptr->inbuf);

            int len = strlen(client_ptr->inbuf);
            message = malloc(len + 1);
            strncpy(message, client_ptr->inbuf, len);
            message[len] = '\0';
            found = 1;
            inbuf -= where;
            memmove(client_ptr->inbuf, client_ptr->inbuf + where, inbuf);
        }

        client_ptr->in_ptr = client_ptr->inbuf + inbuf;
        room = sizeof(client_ptr->inbuf) - inbuf;
        if (found == 1) {
            return message;
        }
    }
    if (nbytes == 0) {
        fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(client_ptr->ipaddr));
	remove_client(clients_list, client_ptr->fd);
    } else if (nbytes == -1) {
        perror("write");
	exit(1);
    }
    return message;
}

// Check if a username is valid
// Valid usernames are ones that are not empty and not the same as an already used username
// Return 0 if valid, 1 if invalid
int validate_username(char *input, struct client *active_clients) {
    if (strlen(input) == 0) {
        return 1;
    }
    
    struct client *curr;
    for (curr = active_clients; curr != NULL; curr = curr->next) {
        if (strcmp(curr->username, input) == 0) {
	    return 1;
	}
    }
    return 0;
}


// Check if input is one of send, show, follow, unfollow, quit
// if input is one of send, show, follow, unfollow command
// return bassed on command:
// send: 1
// show: 2
// follow: 3
// unfollow: 4
// if input is not one of send show follow unfollow, quit then return -1
// if input is a quit command than return 0
int verify_input(char *input) {
   // A correct input will be of the form 
   // **command** rest of the command 
   // or it will be quit
    char *space = strchr(input, ' ');
    if (space == NULL) { // Command is either show, quit or invalid command
        if (strcmp(input, "quit") == 0) {
	    return 0;
	} else if (strcmp(input, SHOW_MSG) == 0) {
	    return 2;
	} else {
	    return -1;
	}
    } else {
	// Isolate part of string before the space
        char before[BUF_SIZE];
	int command_len = strlen(input);
	int rest_len = strlen(space);
	strncpy(before, input, command_len - rest_len);
	before[command_len - rest_len] = '\0';
	// Check if command is send, follow, unfollow, show
 	if (strcmp(before, SEND_MSG) == 0) {
	    return 1;
	} else if (strcmp(before, FOLLOW_MSG) == 0) {
	    return 3;
	} else if (strcmp(before, UNFOLLOW_MSG) == 0) {
	    return 4;
	} else {
	    return -1;
	}
	// Should never get here
	return -1;
    }
}

// Return a pointer to the active user with the corresponding username
struct client * find_user(struct client *active_clients, char *username) {
    struct client *curr;
    for (curr = active_clients; curr != NULL; curr = curr->next) {
        if (strcmp(curr->username, username) == 0) {
	    return curr;
	}
    }
    return NULL;
}

// Add to_follow to client's following list and add client to to_follow's
// followers list
void follow(struct client **clients_list, struct client *client, struct client *to_follow) {
    // Check if client can follow more people
    // Check if to_follow can have more followers
    int client_following = 0;
    int to_follow_followers = 0;
    for (int i = 0; i < FOLLOW_LIMIT; i++) {
        if (client->following[i] != NULL) {
	    client_following++;
	}
	if (to_follow->followers[i] != NULL) {
	    to_follow_followers++;
	}
    }

    if (client_following == FOLLOW_LIMIT) {
	char error[BUF_SIZE];
	if (sprintf(error, "You are following %d users, but you can follow up to %d users.\r\n", 
				client_following, FOLLOW_LIMIT) < 0) {
	    fprintf(stderr, "sprintf error\n");
	}
        if (write(client->fd, error, strlen(error)) == -1) {
	    fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(client->ipaddr));    
	    remove_client(clients_list, client->fd);
	}
	return;
    }
    
    if (to_follow_followers == FOLLOW_LIMIT) {
        char error[BUF_SIZE];
	if (sprintf(error, "This user has too many followers. You can have up to %d followers.\r\n", FOLLOW_LIMIT) < 0) {
	    fprintf(stderr, "sprintf error\n");
	}
	if (write(client->fd, error, strlen(error)) == -1) {
	    fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(client->ipaddr));
	    remove_client(clients_list, client->fd);	    
	}
	return;
    }
    
    // Add to_Follow to client's following list
    int i;
    for (i = 0; i < FOLLOW_LIMIT; i++) {
        if (client->following[i] == NULL) {
	    break;
	}
    }
    client->following[i] = to_follow;

    // Add client to to_follow's follower list
    for (i = 0; i < FOLLOW_LIMIT; i++) {
        if (to_follow->followers[i] == NULL) {
	    break;
	}
    }
    to_follow->followers[i] = client;

    printf("%s is following %s\n", client->username, to_follow->username);
    printf("%s has %s as a follower\n", to_follow->username, client->username);

    // Tell client the follow was sucessfull, tell to_follow they have a new follower
    char following[25 + strlen(to_follow->username)];
    if (sprintf(following, "You are now following %s!\r\n", to_follow->username) < 0) {
        fprintf(stderr, "fprintf error\n");
    }
    if (write(client->fd, following, strlen(following)) == -1) {
	fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(client->ipaddr));
	remove_client(clients_list, client->fd);
	return;
    }
    char follower[20 + strlen(client->username)];
    if (sprintf(follower, "%s has followed you!\r\n", client->username) < 0) {
        fprintf(stderr, "fprintf error\n");
    }
    if (write(to_follow->fd, follower, strlen(follower)) == -1) {
	fprintf(stderr, "Writing to client %s failed\n", inet_ntoa(to_follow->ipaddr));
	remove_client(clients_list, to_follow->fd);
	return;
    }
}

// Remove to_unfollow from client's following list.
// Also remove client from to_follow's follower list.
// Checks if client is actually following to_unfollow
void unfollow(struct client **clients_list, struct client *client, struct client *to_unfollow) {
    int i;
    // Remove follower from client's following list
    for (i = 0; i < FOLLOW_LIMIT; i++) {
	if (client->following[i] != NULL) {
            if (client->following[i]->fd == to_unfollow->fd) {
	        break;
	    }
	}
    }

    // Either we found the one we wanted, or i = FOLLOW_LIMIT
    // If i = follow_limit, then that means the client is not following to_unfollow
    if (i == FOLLOW_LIMIT) {
        char error[strlen(to_unfollow->username) + 25];
	if (sprintf(error, "You are not following %s!\r\n", to_unfollow->username) < 0) {
	    fprintf(stderr, "sprintf error\n");
	}
	if (write(client->fd, error, strlen(error)) == -1)  {
	    fprintf(stderr, "Write to client %s failed\n", inet_ntoa(client->ipaddr));
	    remove_client(clients_list, client->fd);
	}
	return;
    }
    
    // We found the one we wanted to remove
    client->following[i] = NULL;

    // Remove client from to_unfollow's follwer list
    for (i = 0; i < FOLLOW_LIMIT; i++) {
        if (to_unfollow->followers[i] != NULL) {
	    if (to_unfollow->followers[i]->fd == client->fd) {
	        break;
	    }
	}
    }
    // Since we already checked if client actually follows to_unfollow above, we don't need to check it again here
    to_unfollow->followers[i] = NULL;

    // Notify client of the successfull unfollow
    char unfollow[22 + strlen(to_unfollow->username)];
    if (sprintf(unfollow, "You have unfollowd %s\r\n", to_unfollow->username) < 0) {
        fprintf(stderr, "sprintf error\n");
    }
    if (write(client->fd, unfollow, strlen(unfollow)) == -1) {
        fprintf(stderr, "writing to client %s failed\n", inet_ntoa(client->ipaddr));
	remove_client(clients_list, client->fd);
	return;
    }
    printf("%s has unfollowed %s\n", client->username, to_unfollow->username);

}

// Display the previously sent messages of those users this user folowwing
void show_messages(struct client **clients_list, struct client *client) {
    for (int i = 0; i < FOLLOW_LIMIT; i++) {
        if (client->following[i] != NULL) {
	    for (int j = 0; j < MSG_LIMIT; j++) {
	        if (strlen((client->following[i])->message[j]) > 0) {
		    char message[9 + strlen((client->following[i])->username) + BUF_SIZE];
		    if (sprintf(message, "%s said: %s\r\n", client->following[i]->username, 
					    (client->following[i])->message[j]) < 0) {
		        fprintf(stderr, "sprintf error\n");
		    }
		    if (write(client->fd, message, (int)strlen(message)) == -1) {
		        fprintf(stderr, "writing to client %s failed\n", inet_ntoa(client->ipaddr));
			remove_client(clients_list, client->fd);
			return;
		    }
		}
	    }
	}
    }

}

// Send message to all of client's followers. 
// Also store the message in client's message history
void send_message(struct client **clients_list, struct client *client, char *message) {
    

    // Store message in client's message history
    // Here we move the string over and the memories space overlaps
    // so we copy the strlen before hand
    int i;
    for (i = 0; i < MSG_LIMIT; i++) {
         if (strlen(client->message[i]) == 0) {
	      break;
	 }
    }
    // Either we found an empty message slot of i = MSG_LIMIT
    // If i = MSG_LIMIT then the client has sent MSG_LIMIT messages already
    if (i == MSG_LIMIT) {
        char error[67];
        if (sprintf(error, "You have sent too many messages! You can send up to %d messages.\r\n", MSG_LIMIT) < 0) {
	    fprintf(stderr, "sprintf error\n");
	}
	if (write(client->fd, error, strlen(error)) == -1) {
	    fprintf(stderr, "Writing to %s failed\n", inet_ntoa(client->ipaddr));
	    remove_client(clients_list, client->fd);
	}
	return;
    }

    // Copy 140 words
    strncpy(client->message[i], message, 140);
    client->message[i][140] = '\0';
    char *announcement = malloc(140 + strlen(client->username) + 2);
    if (announcement == NULL) {
        perror("malloc");
	exit(1);
    }
    if (sprintf(announcement, "%s: %s\r\n", client->username, client->message[i]) < 0) {
        fprintf(stderr, "sprintf error\n");
    }
    announce(clients_list, client->followers, announcement);
    free(announcement);
}
