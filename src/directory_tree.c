#include "directory_tree.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const unsigned MODE = 0777;

void init_node(node_t *node, char *name, node_type_t type) {
    if (name == NULL) {
        name = strdup("ROOT");
        assert(name != NULL);
    }
    node->name = name;
    node->type = type;
}

file_node_t *init_file_node(char *name, size_t size, uint8_t *contents) {
    file_node_t *node = malloc(sizeof(file_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, FILE_TYPE);
    node->size = size;
    node->contents = contents;
    return node;
}

directory_node_t *init_directory_node(char *name) {
    directory_node_t *node = malloc(sizeof(directory_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, DIRECTORY_TYPE);
    node->num_children = 0;
    node->children = NULL;
    return node;
}

void add_child_directory_tree(directory_node_t *dnode, node_t *child) {
    size_t new_num_children = dnode->num_children + 1;
    dnode->children = realloc(dnode->children, sizeof(node_t *) * new_num_children);
    dnode->children[new_num_children - 1] = child;
    dnode->num_children = new_num_children;
    size_t i = new_num_children - 1;

    // Move the new child up the tree to preserve alphabetical ordering
    while (i >= 1) {
        if (strcmp(dnode->children[i]->name, dnode->children[i - 1]->name) > 0) {
            break;
        }
        else {
            dnode->children[i] = dnode->children[i - 1];
            dnode->children[i - 1] = child;
        }
        i--;
    }
}

void print_directory_tree_helper(node_t *node, size_t level) {
    for (size_t i = 0; i < level; i++) {
        printf("    ");
    }
    printf("%s\n", node->name);
    if (node->type == DIRECTORY_TYPE) {
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            print_directory_tree_helper(dnode->children[i], level + 1);
        }
    }
}

void print_directory_tree(node_t *node) {
    print_directory_tree_helper(node, 0);
}

void create_directory_tree_helper(node_t *node, char *prev_path) {
    char *pathname = malloc(sizeof(char) * (strlen(node->name) + strlen(prev_path) + 2));
    assert(pathname != NULL);
    strcpy(pathname, prev_path);
    strcat(pathname, "/");
    strcat(pathname, node->name);

    if (node->type == DIRECTORY_TYPE) {
        mkdir(pathname, MODE);
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            create_directory_tree_helper(dnode->children[i], pathname);
        }
    }
    else if (node->type == FILE_TYPE) {
        file_node_t *fnode = (file_node_t *) node;
        FILE *file = fopen(pathname, "w");
        assert(fwrite(fnode->contents, sizeof(uint8_t), fnode->size, file) ==
               fnode->size);
        assert(fclose(file) == 0);
    }

    free(pathname);
}

void create_directory_tree(node_t *node) {
    create_directory_tree_helper(node, ".");
}

void free_directory_tree(node_t *node) {
    if (node->type == FILE_TYPE) {
        file_node_t *fnode = (file_node_t *) node;
        free(fnode->contents);
    }
    else {
        assert(node->type == DIRECTORY_TYPE);
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            free_directory_tree(dnode->children[i]);
        }
        free(dnode->children);
    }
    free(node->name);
    free(node);
}
