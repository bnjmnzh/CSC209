#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "ftree.h"

struct TreeNode *ftree_helper(const char *fname, char *path);
/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname) {

    /* This function makes heavy use of a helper function ftree_helper to do all the work
     * ftree_helper takes fname and a path to make recursion simpler.
     */

    struct TreeNode *tree_ptr = ftree_helper(fname, "");
    // Error check
    if (tree_ptr == NULL) {
        return NULL;
    }
    
    return tree_ptr;
}

struct TreeNode *ftree_helper(const char *fname, char *path) {
    struct TreeNode* tree_ptr = malloc(sizeof(struct TreeNode));
    if (tree_ptr == NULL) { // Error check malloc
        fprintf(stderr, "Error calling malloc");
	return NULL;
    }
    // Malloc sufficient space for relative path,
    //  fname, /, and null-terminating character
    char *relative_path = malloc(strlen(path) + strlen(fname) + 2);
    if (relative_path == NULL) { // Error check malloc
        fprintf(stderr, "Error calling malloc");
    }
    strcpy(relative_path, path);
    strcat(relative_path, fname);
    struct stat stat_buf; 

    // Check if fname points to an existing entry
    if (lstat(relative_path, &stat_buf) == -1) { 
        fprintf(stderr, "The path (%s) does not point \
			to an existing entry!\n", relative_path);
    }

    tree_ptr->permissions = stat_buf.st_mode & 0777;

    // Malloc space for fname of file tree
    int fname_len = strlen(fname);
    tree_ptr->fname = malloc(fname_len + 1);
    if (tree_ptr->fname == NULL) { // Error check malloc 
        fprintf(stderr, "Error calling malloc");
	return NULL;
    }
    strcpy(tree_ptr->fname, fname);

    // Set contents and next to initially NULL
    tree_ptr->contents = NULL;
    tree_ptr->next = NULL; 

    if (S_ISREG(stat_buf.st_mode)) { // Base case
        // Code to execute for regular file
	strcpy(&(tree_ptr->type), "-");
    }

    else if (S_ISLNK(stat_buf.st_mode)) { // Base case
        // Code to execute for symbolic link 
        strcpy(&(tree_ptr->type), "l");
    }

    else if (S_ISDIR(stat_buf.st_mode)) { // Recursive case
    	// Code to execute for a directory
	strcpy(&(tree_ptr->type), "d");
	
	DIR *d_ptr = opendir(relative_path);
	if (d_ptr == NULL) { // Error check opening directory
	    fprintf(stderr, "Error on opening %s\n", relative_path);
	    return NULL;
	}
	struct dirent *entry_ptr;
	entry_ptr = readdir(d_ptr);

	strcat(relative_path, "/");

	// This recursion uses a tree pointer to keep track of 
	// where it is in the contex of the directory children
	struct TreeNode *next_ptr;
	// set_contents is high when tree_ptr->contents is 
	// NULL to indicate that you can set a child pointer
	int set_contents = 1;
	
	while (entry_ptr != NULL) {
	    if (entry_ptr->d_name[0] != '.') {
		if (set_contents == 1) {
		    tree_ptr->contents = ftree_helper(
				    entry_ptr->d_name, relative_path);
		    next_ptr = tree_ptr->contents;
		    set_contents = 0;
		} else {
		    next_ptr->next = ftree_helper(
				    entry_ptr->d_name, relative_path);
		    next_ptr = next_ptr->next;
		}
	    }
	    
	    entry_ptr = readdir(d_ptr);
	}

	if (closedir(d_ptr) == -1) { // Error check closing the directory
	    fprintf(stderr, "Error on closing %s\n", relative_path);
	    return NULL;
	}
    }
    free(relative_path); // Free the space for the relative path
    return tree_ptr;
}


/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions)
 *
 */
void print_ftree(struct TreeNode *root) {
	
    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    printf("%*s", depth * 2, "");

    if (root->type != 'd') {
	// Regular file or symbolic link
	printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
    } else {
	// Print a directory
        printf("===== %s (%c%o) =====\n", root->fname,
		       	root->type, root->permissions);
	
	struct TreeNode *temp;
	temp = root->contents;
	
	// Increment depth for going down a level
	depth++;
	while (temp != NULL) {
	    print_ftree(temp);
	    temp = temp->next;
	}
	// Decrement depth for going up a level
	depth--;
	
    }
}


/* 
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 * 
 */
void deallocate_ftree (struct TreeNode *node) {
    
    // Free name of node regardless of type of node
    free(node->fname);
    if (node->type != 'd') { 
	// Base case since regular files and links do not have contents
	free(node);
    } else {
	// This recursion uses two pointers, one to the current node 
	// and one to the previous node to traverse a directorys
	// children and deallocate memory
	struct TreeNode *curr_ptr = node->contents;
	struct TreeNode *prev_ptr = NULL;
	
	while (curr_ptr != NULL) {
	    prev_ptr = curr_ptr;
	    curr_ptr = curr_ptr->next;
	    deallocate_ftree(prev_ptr);
	}
	free(node);
    }
}
