#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#define BLOCK_SIZE 512
#define OCTAL 8
#define ONE 1
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SUPPORTED_OPTION_COUNT 2


/**
 * Header struct.
 */
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

size_t number_of_content_blocks(char size_as_string[]) {
    long size_as_long = strtol(size_as_string, NULL, OCTAL);
    // Size should never be negative.
    assert(size_as_long >= 0);

    size_t size = (size_t) size_as_long;
    return (size / BLOCK_SIZE) + ((size % BLOCK_SIZE == 0) ? 0 : 1);
}

// TODO: some attribute to say that we never return from this func?
void my_errx(int return_code, char return_string[]) {
    printf(return_string);
    exit(return_code);
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
    return result == 0 ? true : false;
}


enum active_option {
    NONE,
    F,
    T,
    V,
    X
};

int main(int argc, char *argv[]) {
    if (argc < 4) {
        my_errx(2, "Need at least 3 arguments.\n");
    }
    char *filename = NULL;
    // Current active option.
    enum active_option active_o = NONE;
    // 0th index:'f', 1st index:'t'
    // char option_decoder[] = {'f', 't'};
    bool encountered_options[SUPPORTED_OPTION_COUNT] = {false};
    // 't' option files
    const char *t_names[argc - 3];
    int t_names_actual_length = 0;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strlen(argv[i]) != 2) my_errx(2, "Long option encountered.\n");
            switch (argv[i][1]) {
                case 'f':
                    encountered_options[0] = true;
                    active_o = F;
                    continue;
                case 't':
                    encountered_options[1] = true;
                    active_o = T;
                    continue;
                default:
                    my_errx(2, "Unknown option.\n");
            }
        }
        switch (active_o) {
            case F:
                filename = argv[i];
                // If there was a 't' switch back to it.
                active_o = encountered_options[1] ? T : NONE;
                break;
            case T:
                // Add search name to the list.
                t_names[t_names_actual_length] = argv[i];
                ++t_names_actual_length;
                break;
            case V:
            case X:
                my_errx(2, "Option not implemented.\n");
                assert(false);
            case NONE:
                my_errx(2, "Option missing!\n");
                assert(false);
        }
    }

    // Check option dependency
    for (size_t i = 0; i < ARRAY_SIZE(encountered_options); ++i) {
        if (!encountered_options[i]) {
            my_errx(2, "Some option wasn't specified.\n");
        }
    }

    bool appearance[t_names_actual_length];

    // Open a tar file.
    FILE *tar_file = fopen(filename, "rb");
    // Some problem with file.
    if (tar_file == NULL) my_errx(2, "File couldn't be opened/wasn't found.\n");
    // Malloc space for header
    whole_header *header = malloc(sizeof(whole_header));
    if (header == NULL) {
        fclose(tar_file);
        my_errx(2, "Malloc returned NULL.\n");
    }

    // Number of empty blocks encountered.
    int empty_block_count = 0;
    while (true) {
        // We found end of archive.
        if (empty_block_count == 2) {
            break;
        }
        // Read header
        size_t header_read_res = fread(header, sizeof(*header), ONE, tar_file);
        if (header_read_res != ONE) {
            // We reached EOF and there was only one empty block.
            if (empty_block_count != 0) {
                free(header);
                fclose(tar_file);
                my_errx(0, "Only one empty block found!\n");
            }
            free(header);
            fclose(tar_file);
            my_errx(2, "Unexpected EOF in archive!\n");
        }

        // Optimization - only if name is empty we check if block was empty.
        if (header->name[0] == '\0') {
            if (is_block_empty(header)) {
                ++empty_block_count;
                continue;
            }
            free(header);
            fclose(tar_file);
            my_errx(2, "Non recognizable header.\n");
        }

        // TODO: we encountered empty block but there wasn't a second one or EOF
        if (empty_block_count != 0) {
            // TODO: should we return?
            free(header);
            fclose(tar_file);
            my_errx(0, "A lone zero block encountered.\n");
        }

        // We only care about regular files.
        if (header->typeflag != '0') {
            free(header);
            fclose(tar_file);
            const char *non_formatted = "mytar: Unsupported header type: %d\n";
            const int formatted_len = (int) strlen(non_formatted) + 4;
            char formatted[formatted_len];
            int res = sprintf(formatted, non_formatted, header->typeflag);
            assert(res == formatted_len);
            my_errx(2, formatted);
        }

        if (t_names_actual_length != 0) {
            // Check that name is in the list.
            // TODO: do this in function.
            for (int i = 0; i < t_names_actual_length; ++i) {
                // we found a match.
                if (strcmp(header->name, t_names[i]) == 0) {
                    appearance[i] = true;
                    printf("%s\n", header->name);
                    // TODO: should there be a break?
                    break;
                }
            }
        } else {
            printf("%s\n", header->name);;;
        }


        // get and skip all the content
        size_t blocks_to_skip = number_of_content_blocks(header->size);

        int skipped_res = fseek(tar_file, (long) (BLOCK_SIZE * blocks_to_skip), SEEK_CUR);
        // We reached the EOF sooner than we should.
        if (skipped_res != 0) {
            // TODO: hopefully right?
            free(header);
            fclose(tar_file);
            my_errx(2, "Unexpected EOF in archive\n");
        }
    }
    bool all_files_found = true;
    for (size_t i = 0; i < ARRAY_SIZE(appearance); ++i) {
        if (!appearance[i]) {
            printf("mytar: %s: Not found in archive\n", t_names[i]);
            all_files_found = false;
        }
    }
    free(header);
    fclose(tar_file);
    if (!all_files_found) my_errx(2, "mytar: Exiting with failure status due to previous errors\n");
}
