#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "directory_tree.h"
#include "fat16.h"

const size_t MASTER_BOOT_RECORD_SIZE = 0x20B;

void follow(FILE *disk, directory_node_t *node, bios_parameter_block_t bpb) {
    directory_entry_t entry;
    assert(fread(&entry, sizeof(directory_entry_t), 1, disk) == 1); // read entry
    // Loop through disk until entry starts with \0
    while (entry.filename[0] != '\0') {
        // Find where on disk contents of entry are located
        size_t offset = get_offset_from_cluster(entry.first_cluster, bpb);
        long loc = ftell(disk);
        fseek(disk, (long) offset, SEEK_SET);
        if (!is_hidden(entry) && !is_directory(entry)) { // is file
            char *contents = malloc(sizeof(char) * entry.file_size);
            assert(fread(contents, entry.file_size, 1, disk) == 1);
            file_node_t *fnode = init_file_node(get_file_name(entry), entry.file_size,
                                                (uint8_t *) contents);
            add_child_directory_tree(node, (node_t *) fnode);
        }
        else if (!is_hidden(entry)) { // is directory
            directory_node_t *dnode = init_directory_node(get_file_name(entry));
            add_child_directory_tree(node, (node_t *) dnode);
            follow(disk, dnode, bpb);
        }
        // Move the disk pointer to the next entry
        fseek(disk, loc, SEEK_SET);
        assert(fread(&entry, sizeof(directory_entry_t), 1, disk) == 1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <image filename>\n", argv[0]);
        return 1;
    }

    FILE *disk = fopen(argv[1], "r");
    if (disk == NULL) {
        fprintf(stderr, "No such image file: %s\n", argv[1]);
        return 1;
    }

    bios_parameter_block_t bpb;

    // Skip past the master boot record
    fseek(disk, MASTER_BOOT_RECORD_SIZE, SEEK_SET);
    // Read the bios parameter block
    assert(fread(&bpb, sizeof(bios_parameter_block_t), 1, disk) == 1);
    // Skip past the padding and file allocation tables to the
    // root directory entries block
    fseek(disk, get_root_directory_location(bpb), SEEK_SET);

    directory_node_t *root = init_directory_node(NULL);
    follow(disk, root, bpb);
    print_directory_tree((node_t *) root);
    create_directory_tree((node_t *) root);
    free_directory_tree((node_t *) root);

    int result = fclose(disk);
    assert(result == 0);
}
