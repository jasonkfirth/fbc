/*
    Project: FreeBASIC AROS test support
    ------------------------------------

    File: tests/aros/resource-unpack.c

    Purpose:

        Expand a sequential ustar resource archive into a writable AROS tree.

    Responsibilities:

        - validate the archive paths before using them
        - create directory entries below one caller-supplied destination
        - copy regular files without loading the archive into memory
        - report bounded diagnostics through the AROS kernel debug channel

    This file intentionally does NOT contain:

        - emulator control
        - test selection
        - architecture-specific compiler flags

    CD directory traversal is unusually expensive on the AROS test images.
    Reading one archive sequentially avoids that cost while still placing the
    extracted test fixtures on writable RAM storage.
*/

#include <proto/debug.h>

#include <dos/dos.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

/* ------------------------------------------------------------------------- */
/* Ustar layout and bounded parsing                                          */
/* ------------------------------------------------------------------------- */

#define TAR_BLOCK_SIZE 512U
#define TAR_NAME_SIZE 100U
#define TAR_PREFIX_SIZE 155U
#define OUTPUT_PATH_SIZE 512U

struct tar_header {
    char name[100];
    char mode[8];
    char user_id[8];
    char group_id[8];
    char size[12];
    char modified_time[12];
    char checksum[8];
    char type;
    char link_name[100];
    char magic[6];
    char version[2];
    char user_name[32];
    char group_name[32];
    char device_major[8];
    char device_minor[8];
    char prefix[155];
    char padding[12];
};

static void report_error(const char *message)
{
    KPutStr((CONST_STRPTR)"FBC_AROS_TEST: resource-unpack: ");
    KPutStr((CONST_STRPTR)message);
    KPutStr((CONST_STRPTR)"\n");
}

static int block_is_empty(const unsigned char *block)
{
    size_t index;

    for (index = 0; index < TAR_BLOCK_SIZE; ++index) {
        if (block[index] != 0) {
            return 0;
        }
    }

    return 1;
}

static int parse_octal_size(const char *text, size_t length, unsigned long *value)
{
    size_t index;
    unsigned long result;

    result = 0;
    for (index = 0; index < length; ++index) {
        unsigned char digit;

        digit = (unsigned char)text[index];
        if ((digit == 0) || (digit == ' ')) {
            continue;
        }
        if ((digit < '0') || (digit > '7')) {
            return 0;
        }
        if (result > ((~0UL - (digit - '0')) >> 3)) {
            return 0;
        }
        result = (result << 3) + (digit - '0');
    }

    *value = result;
    return 1;
}

static int append_header_text(
    char *destination,
    size_t capacity,
    const char *source,
    size_t source_capacity
)
{
    size_t destination_length;
    size_t source_length;

    destination_length = strlen(destination);
    source_length = 0;
    while ((source_length < source_capacity) &&
           (source[source_length] != 0)) {
        ++source_length;
    }

    if (destination_length + source_length >= capacity) {
        return 0;
    }
    memcpy(destination + destination_length, source, source_length);
    destination[destination_length + source_length] = 0;
    return 1;
}

static int archive_path_is_safe(const char *path)
{
    const char *component;

    if ((path[0] == 0) || (path[0] == '/') || (strchr(path, ':') != NULL)) {
        return 0;
    }

    component = path;
    while (*component != 0) {
        const char *separator;
        size_t length;

        separator = strchr(component, '/');
        length = (separator == NULL)
            ? strlen(component)
            : (size_t)(separator - component);
        if ((length == 0) ||
            ((length == 1) && (component[0] == '.')) ||
            ((length == 2) && (component[0] == '.') &&
             (component[1] == '.'))) {
            return 0;
        }
        if (separator == NULL) {
            break;
        }
        component = separator + 1;
    }

    return 1;
}

static int build_archive_path(
    const struct tar_header *header,
    char *path,
    size_t capacity
)
{
    path[0] = 0;
    if (header->prefix[0] != 0) {
        if (!append_header_text(
                path,
                capacity,
                header->prefix,
                TAR_PREFIX_SIZE) ||
            (strlen(path) + 1 >= capacity)) {
            return 0;
        }
        strcat(path, "/");
    }

    if (!append_header_text(path, capacity, header->name, TAR_NAME_SIZE)) {
        return 0;
    }

    while ((path[0] != 0) && (path[strlen(path) - 1] == '/')) {
        path[strlen(path) - 1] = 0;
    }
    return archive_path_is_safe(path);
}

/* ------------------------------------------------------------------------- */
/* Destination creation and archive extraction                              */
/* ------------------------------------------------------------------------- */

static int make_directory(const char *path)
{
    if ((mkdir(path, 0777) == 0) || (errno == EEXIST)) {
        return 1;
    }
    return 0;
}

static int build_output_path(
    const char *destination,
    const char *archive_path,
    char *output_path,
    size_t capacity
)
{
    size_t destination_length;

    destination_length = strlen(destination);
    if (destination_length + strlen(archive_path) + 2 > capacity) {
        return 0;
    }

    strcpy(output_path, destination);
    if ((destination_length != 0) &&
        (destination[destination_length - 1] != '/') &&
        (destination[destination_length - 1] != ':')) {
        strcat(output_path, "/");
    }
    strcat(output_path, archive_path);
    return 1;
}

static int make_parent_directories(char *path)
{
    char *cursor;

    cursor = strchr(path, ':');
    cursor = (cursor == NULL) ? path : cursor + 1;
    while ((cursor = strchr(cursor, '/')) != NULL) {
        *cursor = 0;
        if (!make_directory(path)) {
            *cursor = '/';
            return 0;
        }
        *cursor = '/';
        ++cursor;
    }

    return 1;
}

static int copy_file_data(FILE *archive, FILE *output, unsigned long size)
{
    unsigned char buffer[4096];
    unsigned long remaining;

    remaining = size;
    while (remaining != 0) {
        size_t chunk_size;

        chunk_size = (remaining < sizeof(buffer))
            ? (size_t)remaining
            : sizeof(buffer);
        if (fread(buffer, 1, chunk_size, archive) != chunk_size) {
            return 0;
        }
        if (fwrite(buffer, 1, chunk_size, output) != chunk_size) {
            return 0;
        }
        remaining -= (unsigned long)chunk_size;
    }

    return 1;
}

static int skip_padding(FILE *archive, unsigned long size)
{
    unsigned char padding[TAR_BLOCK_SIZE];
    size_t padding_size;

    padding_size = (size_t)((TAR_BLOCK_SIZE - (size % TAR_BLOCK_SIZE)) %
                            TAR_BLOCK_SIZE);
    return (padding_size == 0) ||
           (fread(padding, 1, padding_size, archive) == padding_size);
}

static int extract_entry(
    FILE *archive,
    const struct tar_header *header,
    const char *destination
)
{
    char archive_path[OUTPUT_PATH_SIZE];
    char output_path[OUTPUT_PATH_SIZE];
    unsigned long size;
    FILE *output;

    if (!build_archive_path(header, archive_path, sizeof(archive_path)) ||
        !build_output_path(
            destination,
            archive_path,
            output_path,
            sizeof(output_path)) ||
        !parse_octal_size(header->size, sizeof(header->size), &size)) {
        report_error("invalid archive entry");
        return 0;
    }

    if (header->type == '5') {
        if ((size != 0) || !make_parent_directories(output_path) ||
            !make_directory(output_path)) {
            report_error("could not create directory");
            return 0;
        }
        return 1;
    }

    if ((header->type != 0) && (header->type != '0')) {
        report_error("unsupported archive entry type");
        return 0;
    }
    if (!make_parent_directories(output_path)) {
        report_error("could not create parent directory");
        return 0;
    }

    output = fopen(output_path, "wb");
    if (output == NULL) {
        report_error("could not create output file");
        return 0;
    }
    if (!copy_file_data(archive, output, size)) {
        fclose(output);
        report_error("could not copy file data");
        return 0;
    }
    if (fclose(output) != 0) {
        report_error("could not close output file");
        return 0;
    }
    if (!skip_padding(archive, size)) {
        report_error("truncated archive padding");
        return 0;
    }

    return 1;
}

static int extract_archive(FILE *archive, const char *destination)
{
    union {
        struct tar_header header;
        unsigned char bytes[TAR_BLOCK_SIZE];
    } block;

    for (;;) {
        if (fread(block.bytes, 1, sizeof(block.bytes), archive) !=
            sizeof(block.bytes)) {
            report_error("truncated archive header");
            return 0;
        }
        if (block_is_empty(block.bytes)) {
            return 1;
        }
        if ((memcmp(block.header.magic, "ustar", 5) != 0) ||
            !extract_entry(archive, &block.header, destination)) {
            if (memcmp(block.header.magic, "ustar", 5) != 0) {
                report_error("archive is not ustar");
            }
            return 0;
        }
    }
}

int main(int argc, char **argv)
{
    FILE *archive;
    int succeeded;

    if (argc != 3) {
        report_error("requires an archive and destination");
        return RETURN_ERROR;
    }
    if (!make_directory(argv[2])) {
        report_error("could not create destination");
        return RETURN_ERROR;
    }

    archive = fopen(argv[1], "rb");
    if (archive == NULL) {
        report_error("could not open archive");
        return RETURN_ERROR;
    }
    succeeded = extract_archive(archive, argv[2]);
    if (fclose(archive) != 0) {
        report_error("could not close archive");
        succeeded = 0;
    }

    return succeeded ? RETURN_OK : RETURN_ERROR;
}

/* end of tests/aros/resource-unpack.c */
