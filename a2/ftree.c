#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
// Add your system includes here.

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

    // Your implementation here.

    // Hint: consider implementing a recursive helper function that
    // takes fname and a path.  For the initial call on the 
    // helper function, the path would be "", since fname is the root
    // of the FTree.  For files at other depths, the path would be the
    // file path from the root to that file.
    
    char path[100] = "";
    struct TreeNode *tree_ptr = ftree_helper(fname, path);
    if (tree_ptr == NULL) {
        return NULL;
    }
    
    return tree_ptr;
}

struct TreeNode *ftree_helper(const char *fname, char *path) {
    struct TreeNode* tree_ptr = malloc(sizeof(struct TreeNode));
    if (tree_ptr == NULL) {
        fprintf(stderr, "Error calling malloc");
	return NULL;
    }
   
    strcat(path, fname);
    struct stat stat_buf; 

    if (lstat(path, &stat_buf) == -1) {
    	fprintf(stderr, "The path (%s) does not point to an existing entry!\n", path);
    }

    tree_ptr->permissions = stat_buf.st_mode & 0777;

    int fname_len = strlen(fname);
    tree_ptr->fname = malloc(fname_len + 1);
    if (tree_ptr->fname == NULL) {
        fprintf(stderr, "Error calling malloc");
	return NULL;
    }
    strcpy(tree_ptr->fname, (char *)fname);

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
	
	DIR *d_ptr = opendir(path);
	if (d_ptr == NULL) {
	    fprintf(stderr, "Error on opening %s\n", path);
	    return NULL;
	}
	struct dirent *entry_ptr;
	entry_ptr = readdir(d_ptr);

	strcat(path, "/");
	int len = strlen(path);

	struct TreeNode *next_ptr;
	int set_contents = 1;
	
	while (entry_ptr != NULL) {
	    if (entry_ptr->d_name[0] != '.') {
		if (set_contents == 1) {
		    tree_ptr->contents = malloc(sizeof(struct TreeNode *));
		    if (tree_ptr->contents == NULL) {
		        fprintf(stderr, "Error calling malloc");
			return NULL;
		    }
		    tree_ptr->contents = ftree_helper(entry_ptr->d_name, path);
		    next_ptr = tree_ptr->contents;
		    set_contents = 0;
		} else {
		    next_ptr->next = malloc(sizeof(struct TreeNode *));
		    if (next_ptr->next == NULL) {
		        fprintf(stderr, "Error calling malloc");
			return NULL;
		    }
		    next_ptr->next = ftree_helper(entry_ptr->d_name, path);
		    next_ptr = next_ptr->next;
		}
		path[len] = '\0';
	    }
	    
	    entry_ptr = readdir(d_ptr);
	}

	if (closedir(d_ptr) == -1) {
	    fprintf(stderr, "Error on closing %s\n", path);
	    return NULL;
	}
	path[len - 1] = '\0';
    }
    
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

    // Your implementation here.
    if (root->contents == NULL) { // Regular file or symbolic link
	printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
    } else {
	// Print a directory
        printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
	
	struct TreeNode *temp;
	if (root->contents != NULL) {
	    temp = root->contents;
	}
	depth++;
	while (temp != NULL) {
	    print_ftree(temp);
	    temp = temp->next;
	}
	depth--;
	
    }
}


/* 
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 * 
 */
void deallocate_ftree (struct TreeNode *node) {
   
    // Your implementation here.
}
