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
    
    char path[50] = "";
    struct TreeNode *tree_ptr = ftree_helper(fname, path);
    if (tree_ptr == NULL) {
        return NULL;
    }
    return tree_ptr;
}

struct TreeNode *ftree_helper(const char *fname, char *path) {
    struct TreeNode* tree_ptr = malloc(sizeof(struct TreeNode));
   
    strcat(path, fname);
    struct stat stat_buf; 

    if (lstat(path, &stat_buf) == -1) {
    	fprintf(stderr, "The path (%s) does not point to an existing entry!\n", path);
    }

    tree_ptr->permissions = stat_buf.st_mode & 0777;
    
    tree_ptr->fname = (char *)fname;
    tree_ptr->contents = NULL;
    tree_ptr->next = NULL; 

    if (S_ISREG(stat_buf.st_mode)) { // Base case
        // Code to execute for regular file
	printf("encounted regular file %s\n", path);
	strcpy(&(tree_ptr->type), "-");
    }

    else if (S_ISLNK(stat_buf.st_mode)) { // Base case
        // Code to execute for symbolic link 
	printf("encountered symbolic link %s\n", path);
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
	
	tree_ptr->contents = malloc(sizeof(struct TreeNode *));
	tree_ptr->next = malloc(sizeof(struct TreeNode *));
	
	char temp_path[50];
	strcpy(temp_path, path);
	strcat(temp_path, "/");
	
	int len = strlen(temp_path);

	while (entry_ptr != NULL) {
	    if (entry_ptr->d_name[0] != '.') {
		tree_ptr->next = ftree_helper(entry_ptr->d_name, temp_path);
	        temp_path[len] = '\0';
	    }
	    entry_ptr = readdir(d_ptr);
	}

	if (closedir(d_ptr) == -1) {
	    fprintf(stderr, "Error on closing %s\n", path);
	    return NULL;
	}
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
        printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
    }
}


/* 
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 * 
 */
void deallocate_ftree (struct TreeNode *node) {
   
   // Your implementation here.
   if (node->contents != NULL) {
	return;
   }
   free(node);
}
