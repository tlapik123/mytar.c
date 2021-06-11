#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>


#define NAME_SIZE 100
#define BLOCK_SIZE 512
#define OCTAL 8
#define ONE 1


typedef struct {                  /* byte offset */
    char name[100];               /*   0 */
    char mode[8];                 /* 100 */
    char uid[8];                  /* 108 */
    char gid[8];                  /* 116 */
    char size[12];                /* 124 */
    char mtime[12];               /* 136 */
    char chksum[8];               /* 148 */
    char typeflag;                /* 156 */
    char linkname[100];           /* 157 */
    char magic[6];                /* 257 */
    char version[2];              /* 263 */
    char uname[32];               /* 265 */
    char gname[32];               /* 297 */
    char devmajor[8];             /* 329 */
    char devminor[8];             /* 337 */
    char prefix[155];             /* 345 */
    /* 500 */
} posix_header;

enum YesNoEOF {
    YES,
    NO,
    END_OF_FILE,
};

inline int number_of_content_blocks(char size_as_string[]) {
    long size_as_long = strtol(size_as_string, NULL, OCTAL);
    return (size_as_long / BLOCK_SIZE) + ((size_as_long % BLOCK_SIZE == 0) ? 0 : 1);
}

/**
 * Check if one BLOCK_SIZEd block has content.
 * @param file_handle File to read block from.
 * @return
 * True: Some content was read.
 * False: Some content was read;
 */
enum YesNoEOF was_content_in_block(FILE *file_handle) {
    unsigned long long content = 0;
    // Read 512 block - one small part at a time to tell if its empty
    unsigned long long tmp_content;
    int how_many_times = BLOCK_SIZE / sizeof(tmp_content);
    while (how_many_times > 0) {
        size_t num_of_suc_read_elements = fread(&tmp_content, sizeof(tmp_content), 1, file_handle);
        // TODO EOF was reached
        if (num_of_suc_read_elements != 1) {
            return END_OF_FILE;
        }
        --how_many_times;
        content |= tmp_content;
    }
    return content == 0 ? NO : YES;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("No arguments were supplied\n");
        return 1;
    }
    // Open a tar file.
    FILE *tar_file = fopen(argv[1], "rb");
    // Malloc space for header
    posix_header *header = malloc(sizeof(posix_header));

    while (true) {
        size_t result = fread(header, sizeof(*header), ONE, tar_file);
        // TODO: something went wrong.
        if (result != ONE) return 2;

        // TODO check that name is in the list.

        printf(header->name);

        // get and skip all the content
        int blocks_to_skip = number_of_content_blocks(header->size);

    }
}
