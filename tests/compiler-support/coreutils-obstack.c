/*
    Project: FreeBASIC Coreutils library regression
    File: coreutils-obstack.c
    Purpose: Check the complete translated obstack through its public exports.
    Responsibilities: Verify layout, allocation, growth, cleanup and callbacks.
    The BASIC fixture supplies the production implementation and canonical types.
*/
#include <limits.h>
#include <setjmp.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

/* The selected GNU interface uses size_t for chunk sizes and alignment masks.
   Keep its canonical layout here independently of the BASIC declarations.
   A system obstack header can select an older interface with narrower fields. */
struct obstack_chunk {
    char *limit;
    struct obstack_chunk *prev;
};
struct obstack {
    size_t chunk_size;
    struct obstack_chunk *chunk;
    char *object_base, *next_free, *chunk_limit;
    union { size_t tempint; char *tempptr; } temp;
    size_t alignment_mask;
    union { void *(*plain)(size_t); void *(*extra)(void *, size_t); } chunkfun;
    union { void (*plain)(void *); void (*extra)(void *, void *); } freefun;
    void *extra_arg;
    unsigned use_extra_arg : 1;
    unsigned maybe_empty_object : 1;
    unsigned alloc_failed : 1;
};

int rpl_obstack_begin(struct obstack *, size_t, size_t,
    void *(*)(size_t), void (*)(void *));
int rpl_obstack_begin_1(struct obstack *, size_t, size_t,
    void *(*)(void *, size_t), void (*)(void *, void *), void *);
void rpl_obstack_newchunk(struct obstack *, size_t);
int rpl_obstack_allocated_p(struct obstack *, void *);
void rpl_obstack_free(struct obstack *, void *);
size_t rpl_obstack_memory_used(struct obstack *);
void (*fb_test_obstack_set_handler(void (*)(void)))(void);
void fb_test_obstack_layout(unsigned long long *);
unsigned fb_test_obstack_flags(struct obstack *);
extern int exit_failure;
int exit_failure = 1;

/* Each serial run owns at most 128 chunks, with a 1 MiB request ceiling. */
enum { maximum_chunks = 128, maximum_request = 1024 * 1024 };
struct allocation { void *address; size_t bytes; };
static struct allocation allocations[maximum_chunks];
static size_t allocation_calls, active_bytes;
static int fail_next, extra_cookie, notification_fd = -1, failure_kind, oversized_request;
static unsigned long cases, checks;
static jmp_buf escape;

static void check(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "OBSTACK_FAIL case=%lu check=%lu\n", cases, checks);
        exit(1);
    }
}

static void *allocate_chunk(size_t bytes)
{
    allocation_calls++;
    if (fail_next) {
        fail_next = 0;
        return NULL;
    }
    if (bytes > maximum_request) {
        check(oversized_request);
        return NULL;
    }
    for (size_t index = 0; index < maximum_chunks; index++) {
        if (allocations[index].address == NULL) {
            void *address = malloc(bytes);
            check(address != NULL);
            allocations[index] = (struct allocation){address, bytes};
            active_bytes += bytes;
            return address;
        }
    }
    check(0);
    return NULL;
}

static void release_chunk(void *address)
{
    for (size_t index = 0; index < maximum_chunks; index++) {
        if (allocations[index].address == address) {
            active_bytes -= allocations[index].bytes;
            allocations[index] = (struct allocation){0};
            free(address);
            return;
        }
    }
    check(0);
}

static void *allocate_extra(void *argument, size_t bytes)
{
    check(argument == &extra_cookie);
    return allocate_chunk(bytes);
}

static void release_extra(void *argument, void *address)
{
    check(argument == &extra_cookie);
    release_chunk(address);
}

static int begin(struct obstack *value, size_t bytes, size_t alignment, int extra)
{
    return extra ? rpl_obstack_begin_1(value, bytes, alignment,
        allocate_extra, release_extra, &extra_cookie) :
        rpl_obstack_begin(value, bytes, alignment, allocate_chunk, release_chunk);
}

static void check_layout(void)
{
    unsigned long long observed[9] = {0};
    const size_t expected[] = {sizeof(struct obstack), offsetof(struct obstack, chunk),
        offsetof(struct obstack, object_base), offsetof(struct obstack, next_free),
        offsetof(struct obstack, chunk_limit), offsetof(struct obstack, alignment_mask),
        offsetof(struct obstack, chunkfun), offsetof(struct obstack, freefun),
        offsetof(struct obstack, extra_arg)};
    fb_test_obstack_layout(observed);
    for (size_t index = 0; index < sizeof(expected) / sizeof(expected[0]); index++)
        check(observed[index] == expected[index]);
}

static size_t logical_chunk_bytes(struct obstack *value)
{
    size_t total = 0, chunks = 0;
    for (struct obstack_chunk *chunk = value->chunk; chunk != NULL; chunk = chunk->prev) {
        size_t index, bytes;
        check(++chunks <= maximum_chunks);
        for (index = 0; index < maximum_chunks; index++)
            if (allocations[index].address == chunk)
                break;
        check(index < maximum_chunks);
        bytes = (size_t)(chunk->limit - (char *)chunk);
        /* check() exits on a failed lookup; the index is in the allocation table. */
        // cppcheck-suppress arrayIndexOutOfBounds
        check(bytes <= allocations[index].bytes);
        total += bytes;
    }
    /* Alignment can leave unused bytes at the end of an allocated block. */
    check(total <= active_bytes);
    return total;
}

static void successful_allocations(void)
{
    const size_t sizes[] = {0, 32, 128, 512, 4096};
    const size_t alignments[] = {0, 1, 2, 4, 8, 16, 32, 64};
    const size_t growth[] = {0, 1, 15, 100, 2000, 32768};
    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        for (size_t a = 0; a < sizeof(alignments) / sizeof(alignments[0]); a++) {
            for (int extra = 0; extra <= 1; extra++) {
                for (unsigned empty = 0; empty <= 1; empty++) {
                    struct obstack value = {0};
                    cases++;
                    check(active_bytes == 0);
                    check(begin(&value, sizes[s], alignments[a], extra) == 1);
                    check(value.chunk != NULL && value.next_free == value.object_base);
                    check(value.use_extra_arg == (unsigned)extra);
                    for (size_t g = 0; g < sizeof(growth) / sizeof(growth[0]); g++) {
                        unsigned char original[47];
                        size_t available = (size_t)(value.chunk_limit - value.next_free);
                        size_t length = available < sizeof(original) ? available : sizeof(original);
                        value.object_base = value.next_free;
                        for (size_t index = 0; index < length; index++)
                            original[index] = (unsigned char)(index + g + s);
                        /* Exactly length bytes are initialized above; length zero reads none. */
                        // cppcheck-suppress uninitvar
                        memcpy(value.object_base, original, length);
                        value.next_free += length;
                        value.maybe_empty_object = empty != 0;
                        rpl_obstack_newchunk(&value, growth[g]);
                        check(value.chunk != NULL);
                        check((uintptr_t)value.object_base % (value.alignment_mask + 1u) == 0);
                        check(value.next_free == value.object_base + length);
                        check((size_t)(value.chunk_limit - value.next_free) >= growth[g]);
                        check(memcmp(value.object_base, original, length) == 0);
                        check(rpl_obstack_allocated_p(&value, value.object_base) != 0);
                        check(rpl_obstack_allocated_p(&value, &extra_cookie) == 0);
                        check(rpl_obstack_memory_used(&value) == logical_chunk_bytes(&value));
                        check(fb_test_obstack_flags(&value) == (unsigned)extra);
                    }
                    rpl_obstack_free(&value, NULL);
                    check(active_bytes == 0);
                }
            }
        }
    }
}

static void failure_callback(void)
{
    const unsigned char notification = 1;
    if (notification_fd >= 0)
        check(write(notification_fd, &notification, 1) == 1);
    if (failure_kind == 0)
        _Exit(73);
    if (failure_kind == 1)
        longjmp(escape, 1);
    /* The returning callback deliberately violates the GNU contract. */
}

static void __attribute__((noreturn)) failure_child(int extra, int growing, int overflow, int kind,
    void (*original_handler)(void))
{
    /* Static storage retains defined contents when the callback longjmps. */
    static struct obstack value;
    struct obstack_chunk *volatile old_chunk = NULL;
    size_t calls_before;
    failure_kind = kind;
    oversized_request = overflow;
    check(active_bytes == 0);
    if (growing) {
        check(begin(&value, 256, 16, extra) == 1);
        old_chunk = value.chunk;
    }
    calls_before = allocation_calls;
    if (kind == 3) {
        fb_test_obstack_set_handler(original_handler);
        exit_failure = 41;
    } else {
        fb_test_obstack_set_handler(failure_callback);
    }
    if (setjmp(escape) == 0) {
        fail_next = !overflow;
        if (growing)
            rpl_obstack_newchunk(&value, overflow ? SIZE_MAX : 512);
        else
            begin(&value, overflow ? SIZE_MAX : 256, 16, extra);
        check(0);
    }
    check(kind == 1);
    check(allocation_calls == calls_before + (overflow && growing ? 0u : 1u));
    check(value.chunk == old_chunk);
    if (growing)
        rpl_obstack_free(&value, NULL);
    check(active_bytes == 0);
    _Exit(0);
}

static void failing_allocations(void (*original_handler)(void), int valid_only)
{
    const struct rlimit no_core = {0, 0};
    check(setrlimit(RLIMIT_CORE, &no_core) == 0);
    for (int extra = 0; extra <= 1; extra++) {
        for (int growing = 0; growing <= 1; growing++) {
            for (int overflow = 0; overflow <= 1; overflow++) {
                for (int kind = 0; kind <= 3; kind++) {
                    int descriptors[2], status;
                    pid_t child;
                    ssize_t received;
                    unsigned char notifications[2] = {0};
                    if (valid_only && kind == 2)
                        continue;
                    cases++;
                    check(pipe(descriptors) == 0);
                    child = fork();
                    check(child >= 0);
                    if (child == 0) {
                        close(descriptors[0]);
                        notification_fd = descriptors[1];
                        failure_child(extra, growing, overflow, kind, original_handler);
                    }
                    close(descriptors[1]);
                    received = read(descriptors[0], notifications, sizeof(notifications));
                    close(descriptors[0]);
                    check(waitpid(child, &status, 0) == child);
                    check(received == (kind == 3 ? 0 : 1));
                    check(kind == 3 || notifications[0] == 1);
                    if (kind == 2)
                        check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
                    else
                        check(WIFEXITED(status) && WEXITSTATUS(status) == (kind == 0 ? 73 : kind == 3 ? 41 : 0));
                    check(active_bytes == 0);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    int valid_only = argc == 2 && strcmp(argv[1], "--valid-only") == 0;
    void (*original_handler)(void) = fb_test_obstack_set_handler(failure_callback);
    check(argc == 1 || valid_only);
    check_layout();
    successful_allocations();
    failing_allocations(original_handler, valid_only);
    fb_test_obstack_set_handler(original_handler);
    printf("OBSTACK_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}

/* end of coreutils-obstack.c */
