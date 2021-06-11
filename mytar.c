#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>


#define NAME_SIZE 100
#define BLOCK_SIZE 512
#define OCTAL 8
#define ONE 1


typedef struct {                    /* byte offset */
    char name[100];                 /*   0 */
    char mode[8];                   /* 100 */
    char uid[8];                    /* 108 */
    char gid[8];                    /* 116 */
    char size[12];                  /* 124 */
    char mtime[12];                 /* 136 */
    char chksum[8];                 /* 148 */
    char typeflag;                  /* 156 */
    char linkname[100];             /* 157 */
    char magic[6];                  /* 257 */
    char version[2];                /* 263 */
    char uname[32];                 /* 265 */
    char gname[32];                 /* 297 */
    char devmajor[8];               /* 329 */
    char devminor[8];               /* 337 */
    char prefix[155];               /* 345 */
    char empty[12];                 /* 500 */
    /* 512 */
} whole_header;

enum YesNoEOF {
    YES,
    NO,
    END_OF_FILE,
};

size_t number_of_content_blocks(char size_as_string[]) {
    long size_as_long = strtol(size_as_string, NULL, OCTAL);
    // Size should never be negative.
    assert(size_as_long > 0);

    size_t size = (size_t) size_as_long;
    return (size / BLOCK_SIZE) + ((size % BLOCK_SIZE == 0) ? 0 : 1);
}


bool is_block_empty(void *block) {
    unsigned long long result = 0;
    unsigned int repeat_count = BLOCK_SIZE / sizeof(result);
    // there shouldn't be any remainder
    assert((BLOCK_SIZE % sizeof(result)) == 0);

    unsigned long long *part = block;
    while (repeat_count > 0) {
        result |= *part;
        // move to next part to check
        ++part;
        --repeat_count;
    }
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("No arguments were supplied\n");
        return 1;
    }
    // Open a tar file.
    FILE *tar_file = fopen(argv[1], "rb");
    // Malloc space for header
    whole_header *header = malloc(sizeof(whole_header));

    // Number of empty blocks encountered.
    int empty_block_count = 0;
    while (true) {
        // Read header
        size_t header_read_res = fread(header, sizeof(*header), ONE, tar_file);
        // TODO: something went wrong. + check empty block rules
        if (header_read_res != ONE) return 2;

        // Optimization - only if name is empty we check if block was empty.
        if (header->name[0] == '\0') {
            if(is_block_empty(header)) {
                ++empty_block_count;
                continue;
            }
            // TODO: no name and empty block shouldn't happen.
            return 2;
        }

        // TODO: we encountered empty block but there wasn't a second one of EOF
        if(empty_block_count != 0){
            return 2;
        }
        // TODO check that name is in the list.

        printf("%s\n", header->name);

        // get and skip all the content
        size_t blocks_to_skip = number_of_content_blocks(header->size);

        size_t skipped_content = fread(header, BLOCK_SIZE, blocks_to_skip, tar_file);
        // We reached the EOF sooner than we should.
        if (skipped_content != blocks_to_skip) {
            // TODO: do more here.
            return 2;
        }
    }
}
