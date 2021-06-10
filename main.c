#include <stdio.h>
#include <stdbool.h>


#define NAME_SIZE 100
#define BLOCK_SIZE 512
#define PART_SIZE (BLOCK_SIZE/ sizeof(long long))


struct posix_header {                              /* byte offset */
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
};

/**
 * Check if one BLOCK_SIZEd block has content.
 * @param file_handle File to read block from.
 * @return
 * 0: No content was read.
 * Else: Some content was read;
 */
unsigned long long read_block_for_content(FILE *file_handle) {
    unsigned long long content = 0;
    // Read 512 block - one small part at a time to tell if its empty
    int how_many_times = PART_SIZE;
    while (how_many_times > 0) {
        unsigned long long tmp_content;
        fread(&tmp_content, PART_SIZE, 1, file_handle);
        --how_many_times;
        content |= tmp_content;
    }
    return content;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("No arguments were supplied\n");
        return 1;
    }
    // Open a tar file.
    FILE *tar_file = fopen(argv[1], "rb");

    // Read name of the first file there.
    char name[NAME_SIZE];
    fgets(name, NAME_SIZE, tar_file);
    printf("%s\n", name);

    // Seek to second block.
    int was_successful = fseek(tar_file, BLOCK_SIZE, SEEK_SET);


    // Have some enum later to tell if 2 blocks were without content.
    int blocks_without_content = 0;
    unsigned long long content = 0;
    while (blocks_without_content != 2){
        content |= read_block_for_content(tar_file);
        if (content == 0){
            ++blocks_without_content;
        } else{
            // zero out content
            content = 0;
        }
    }

    bool block_read_flag = false;





    // If 2 consecutive blocks empty read another name

    // Repeat.



    return 0;
}
