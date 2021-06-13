#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#define BLOCK_SIZE 512
#define OCTAL 8
#define ONE 1
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define SUPPORTED_OPTION_COUNT 2
#define PROGRAM_NAME "mytar"
#define EOF_ERR PROGRAM_NAME": Unexpected EOF in archive\n"
#define NON_RECOVERABLE_ERR PROGRAM_NAME": Error is not recoverable: exiting now\n"


static inline size_t number_of_content_blocks(char size_as_string[]);

__attribute__((noreturn))
static void my_errx(int return_code, char return_string[], int va_count, ...);

static bool is_block_empty(void *block);

static void my_dispose(FILE *file_to_close, void *memory_to_free);

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

static inline size_t number_of_content_blocks(char size_as_string[]) {
    size_t size_as_long = strtoul(size_as_string, NULL, OCTAL);
    return (size_as_long / BLOCK_SIZE) + ((size_as_long % BLOCK_SIZE == 0) ? 0 : 1);
}

/**
 * THIS FUNCTION NEVER RETURNS!
 *
 * Formats and prints error message to stderr and exits with specified return code.
 * @param return_code Code to exit with.
 * @param return_string String to write to stderr.
 * @param va_count Number of variable arguments.
 * @param ... Arguments format the string with.
 */
static void my_errx(int return_code, char return_string[], int va_count, ...) {
    va_list args;
    va_start(args, va_count);
    // print error message to stderr.
    vfprintf(stderr, return_string, args);
    va_end(args);
    exit(return_code);
}

/**
 * Checks if the given block is empty.
 * @param block Block to check.
 * @return
 * True: Block is empty.
 * False: Block is not empty.
 */
static bool is_block_empty(void *block) {
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

// TODO: remove this.
int get_num_of_digits(long num) {
    int digits = 0;
    while (num > 0) {
        ++digits;
        num = num / 10;
    }
    return digits;
}

/**
 * Closes the file and frees the memory.
 * @param file_to_close File in need of closing.
 * @param memory_to_free Memory in need of freeing.
 */
static void my_dispose(FILE *file_to_close, void *memory_to_free) {
    free(memory_to_free);
    int close_res = fclose(file_to_close);
    if (close_res == EOF) my_errx(2, "File wasn't successfully closed!\n", 0);
}

/**
 * What option is active
 */
enum active_option {
    NONE,
    F,
    T,
    V,
    X
};

/**
 * Parse command line arguments.
 * @param argc Number of actual arguments (without program name).
 * @param argv Arguments to parse.
 * @param filename Pointer to Filename to return to.
 * @param t_names_actual_length Actual lenght of t_name array.
 * @param t_names Array of 't' option file names.
 */
static void
parse_options(int argc, char *argv[], const char **filename, int *t_names_actual_length, const char *t_names[]) {
    if (argc < 3) {
        my_errx(2, "Need at least 3 arguments.\n", 0);
    }
    // Current active option.
    enum active_option active_o = NONE;
    // 0th index:'f', 1st index:'t'
    char option_decoder[] = {'f', 't'};
    bool encountered_options[SUPPORTED_OPTION_COUNT] = {false};
    for (int i = 0; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (strlen(argv[i]) != 2) my_errx(2, "Long option %s encountered.\n", 1, argv[i]);
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
                    my_errx(2, "Unknown option.\n", 0);
            }
        }
        switch (active_o) {
            case F:
                *filename = argv[i];
                // If there was a 't' switch back to it.
                active_o = encountered_options[1] ? T : NONE;
                break;
            case T:
                // Add search name to the list.
                t_names[*t_names_actual_length] = argv[i];
                ++(*t_names_actual_length);
                break;
            case V:
            case X:
                my_errx(2, "Option not implemented.\n", 0);
            case NONE:
                my_errx(2, "Option missing!\n", 0);
            default:
                assert(false);
        }
    }

    // Check option dependency
    for (size_t i = 0; i < ARRAY_SIZE(encountered_options); ++i) {
        if (!encountered_options[i]) {
            my_errx(2, "Option %c wasn't specified.\n", 1, option_decoder[i]);
        }
    }
}


int main(int argc, char *argv[]) {
    int t_names_actual_length = 0;
    const char *filename = NULL;
    const char *t_names[argc - 3];
    parse_options(--argc, ++argv, &filename, &t_names_actual_length, t_names);

    bool appearance[t_names_actual_length];

    // Open a tar file.
    FILE *tar_file = fopen(filename, "rb");
    // Some problem with file.
    if (tar_file == NULL) my_errx(2, "File couldn't be opened/wasn't found.\n", 0);
    // Malloc space for header
    whole_header *header = malloc(sizeof(whole_header));
    if (header == NULL) {
        fclose(tar_file);
        my_errx(2, "Malloc returned NULL.\n", 0);
    }

    // Number of empty blocks encountered.
    int empty_block_count = 0;
    size_t blocks_so_far = 0;
    while (true) {
        // We found end of archive.
        if (empty_block_count == 2) {
            break;
        }
        // Read header
        size_t header_read_res = fread(header, ONE, sizeof(*header), tar_file);
        if (header_read_res != sizeof(*header)) {
            // We reached EOF and there was only one empty block.
            if (empty_block_count != 0) {
                my_dispose(tar_file, header);
                my_errx(0, PROGRAM_NAME": A lone zero block at %zu\n", 1, (blocks_so_far + 1));
            }
            // We reached EOF without 2 empty blocks.
            if (header_read_res == 0) break;

            my_dispose(tar_file, header);
            my_errx(2, EOF_ERR NON_RECOVERABLE_ERR, 0);
        }

        // Optimization - only if name is empty we check if block was empty.
        if (header->name[0] == '\0') {
            if (is_block_empty(header)) {
                ++empty_block_count;
                continue;
            }
            my_dispose(tar_file, header);
            my_errx(2, "Non recognizable header.\n", 0);
        }

        // We encountered empty block but there wasn't a second one or EOF
        if (empty_block_count != 0) {
            my_dispose(tar_file, header);
            my_errx(0, PROGRAM_NAME": A lone zero block at %zu\n", 1, (blocks_so_far + 1));
        }

        // We only care about regular files.
        if (header->typeflag != '0' && header->typeflag != 0) {
            my_dispose(tar_file, header);
            my_errx(2, PROGRAM_NAME": Unsupported header type: %d\n", 1, header->typeflag);
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
        size_t size_to_skip = BLOCK_SIZE * blocks_to_skip;

        // Need some variable to read to.
        char tmp_block[size_to_skip];
        size_t skipped_res = fread(tmp_block, ONE, size_to_skip, tar_file);
        // We reached the EOF sooner than we should.
        if (skipped_res != size_to_skip) {
            my_dispose(tar_file, header);
            my_errx(2, EOF_ERR NON_RECOVERABLE_ERR, 0);
        }
        // Increment number of blocks encountered.
        blocks_so_far += (blocks_to_skip + 1);
    }
    bool all_files_found = true;
    for (size_t i = 0; i < ARRAY_SIZE(appearance); ++i) {
        if (!appearance[i]) {
            fprintf(stderr, PROGRAM_NAME": %s: Not found in archive\n", t_names[i]);
            all_files_found = false;
        }
    }
    my_dispose(tar_file, header);
    if (!all_files_found) my_errx(2, PROGRAM_NAME": Exiting with failure status due to previous errors\n", 0);
}
