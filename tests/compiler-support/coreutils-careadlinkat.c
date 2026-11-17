/*
    FreeBASIC translated Gnulib allocator regression
    File coreutils-careadlinkat.c
    Exercise buffer ownership, callback failures and allocator accounting.
    Translation and compiler diagnostic policy belong to the test runner.
*/
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocator.h"

typedef intptr_t (*read_callback)(int, const char *, char *, size_t);
char *careadlinkat(int, const char *, char *, size_t,
                  const struct allocator *, read_callback);
#ifdef _WIN32
int *__errno_location(void);
int *__errno_location(void) { return &errno; }
#endif

/* Each live allocation has sixteen guard bytes on both sides. Only the
   allocator callbacks own these blocks, including failed shrink attempts. */
struct allocation { unsigned char *raw; size_t size; };
static struct allocation blocks[8];
static size_t wanted;
static int calls, allocations, releases, shrinks, deaths;
static int fail_read, fail_allocate, fail_shrink, overflow_probe;

static void require(int condition)
{
    if (!condition) abort();
}

static int block_index(void *value)
{
    for (int index = 0; index < 8; index++)
        if (blocks[index].raw && blocks[index].raw + 16 == value) return index;
    abort();
}

static void check_block(int index)
{
    for (size_t offset = 0; offset < 16; offset++) {
        require(blocks[index].raw[offset] == 0xa5);
        require(blocks[index].raw[16 + blocks[index].size + offset] == 0xa5);
    }
}

static void *new_block(size_t size)
{
    require(size <= 16384);
    for (int index = 0; index < 8; index++) {
        if (blocks[index].raw) continue;
        blocks[index].raw = malloc(size + 32);
        require(blocks[index].raw != NULL);
        blocks[index].size = size;
        memset(blocks[index].raw, 0xa5, size + 32);
        return blocks[index].raw + 16;
    }
    abort();
}

static void *allocate_block(size_t size)
{
    allocations++;
    if (allocations == fail_allocate) return NULL;
    return new_block(size);
}

static void release_block(void *value)
{
    int index = block_index(value);
    check_block(index);
    free(blocks[index].raw);
    blocks[index].raw = NULL;
    releases++;
    errno = 123;
}

static void *shrink_block(void *value, size_t size)
{
    shrinks++;
    int index = block_index(value);
    check_block(index);
    if (fail_shrink) return NULL;
    void *replacement = new_block(size);
    memcpy(replacement, value, size < blocks[index].size ? size : blocks[index].size);
    release_block(value);
    return replacement;
}

static void die_block(size_t size)
{
    require(size > 0 && size <= 16384);
    deaths++;
}

const struct allocator stdlib_allocator = {
    allocate_block, shrink_block, release_block, NULL
};

static intptr_t read_value(int fd, const char *filename, char *buffer, size_t size)
{
    require(fd == 37 && strcmp(filename, "audit") == 0);
    calls++;
    if (overflow_probe) return (intptr_t) size;
    require(size <= 16384);
    if (calls == fail_read) {
        errno = 19;
        return -1;
    }
    size_t count = wanted < size ? wanted : size;
    for (size_t index = 0; index < count; index++)
        buffer[index] = (char) (33 + index % 80);
    return (intptr_t) count;
}

static void exercise(size_t length, size_t capacity, int allocation_failure,
                     int read_failure, int shrink_failure, int mode)
{
    unsigned char caller[1056];
    memset(caller, 0xa5, sizeof caller);
    char *provided = capacity ? (char *) caller + 16 : NULL;
    wanted = length;
    calls = allocations = releases = shrinks = deaths = 0;
    fail_allocate = allocation_failure;
    fail_read = read_failure;
    fail_shrink = shrink_failure;
    overflow_probe = mode == 4;
    struct allocator allocator = stdlib_allocator;
    if (mode == 1) allocator.reallocate = NULL;
    /* A returning DIE callback is a deliberate provider-contract control.
       The normal cases use the allocator's documented null DIE option. */
    if (mode == 3) allocator.die = die_block;
    errno = 0;
    char *value = careadlinkat(37, "audit", provided,
                             overflow_probe ? SIZE_MAX : capacity,
                             mode == 2 ? NULL : &allocator, read_value);
    int observed_errno = errno;
    int kind = value == NULL ? 0 : value == provided ? 1 : 2;
    if (value) {
        require(value[length] == 0);
        for (size_t index = 0; index < length; index++)
            require(value[index] == (char) (33 + index % 80));
        if (kind == 2) check_block(block_index(value));
    }
    for (size_t index = 0; index < 16; index++) require(caller[index] == 0xa5);
    if (!overflow_probe)
        for (size_t index = capacity + 16; index < sizeof caller; index++)
            require(caller[index] == 0xa5);
    printf("%zu %zu %d %d %d %d %d %d %d %d %d %d\n", length, capacity,
           allocation_failure, read_failure, shrink_failure, mode, kind,
           observed_errno, calls, allocations, releases, shrinks);
    require(deaths == (mode == 3 && kind == 0 && observed_errno == 12));
    if (kind == 2) release_block(value);
    for (int index = 0; index < 8; index++) require(blocks[index].raw == NULL);
}

int main(void)
{
    static const size_t lengths[] = {0, 1, 6, 7, 1023, 1024, 1025, 2048, 2049, 4096};
    static const size_t capacities[] = {0, 1, 7, 1024};
    for (size_t l = 0; l < sizeof lengths / sizeof lengths[0]; l++) {
        for (size_t c = 0; c < sizeof capacities / sizeof capacities[0]; c++) {
            for (int a = 0; a < 5; a++) {
                for (int r = 0; r < 3; r++) {
                    for (int s = 0; s < 2; s++) {
                        exercise(lengths[l], capacities[c], a, r, s, 0);
                        exercise(lengths[l], capacities[c], a, r, s, 1);
                        exercise(lengths[l], capacities[c], a, r, s, 2);
                        exercise(lengths[l], capacities[c], a, r, s, 3);
                    }
                }
            }
        }
    }
    exercise(0, 1, 0, 0, 0, 4);
    return 0;
}
/* end of coreutils-careadlinkat.c */
