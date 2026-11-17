/*
    Project FreeBASIC SQLite read-only vector regression
    File sqlite-readonly-vectors.c
    Purpose Observe legacy and read-only callback ABIs from native C.
    Responsibilities Check pointer vectors, outputs and full-width C int values.
    The BASIC fixture owns its buffers and the production legacy adapter.
*/
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

struct sqlite3_vtab;
typedef int (*legacy_callback)(void *, int, char **, char **);
typedef int (*constructor_callback)(void *, void *, int, const char *const *, struct sqlite3_vtab **, char **);

int sqlite3InitCallback(void *, int, char **, char **);
void fb_test_readonly_check(int condition, int line);
void fb_test_readonly_vector_case(int count, int null_context, int null_rows, int null_names, int result);
constructor_callback fb_test_readonly_get_constructor(void);
void fb_test_readonly_init_layout(unsigned long long *length, unsigned long long *field_offset);
int fb_test_readonly_invoke_legacy(legacy_callback callback, void *context, int count, char **values, char **names);
int fb_test_readonly_observe_constructor(void *db, void *auxiliary, int count,
    const char *const *arguments, struct sqlite3_vtab **vtab, char **message);

static unsigned long cases, checks, legacy_calls, constructor_calls;
static const char *const *expected_arguments;
static int expected_count, expected_result;
static int db_tag, auxiliary_tag;
/* The callback returns this aligned borrowed handle without dereferencing it. */
static max_align_t vtab_tag;
static char result_message[] = "constructor result";

/* Match the preceding port record, including its original pointer type. */
struct original_init_info {
    unsigned int new_tnum;
    unsigned char database_index, busy;
    unsigned int flags;
    const char **az_init;
};

void fb_test_readonly_check(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "SQLITE_READONLY_FAIL case=%lu line=%d\n", cases, line);
        exit(1);
    }
}

int fb_test_readonly_invoke_legacy(legacy_callback callback, void *context, int count, char **values, char **names)
{
    fb_test_readonly_check(callback == sqlite3InitCallback, __LINE__);
    legacy_calls++;
    return callback(context, count, values, names);
}

int fb_test_readonly_observe_constructor(void *db, void *auxiliary, int count,
    const char *const *arguments, struct sqlite3_vtab **vtab, char **message)
{
    fb_test_readonly_check(db == &db_tag && auxiliary == &auxiliary_tag, __LINE__);
    fb_test_readonly_check(count == expected_count && arguments == expected_arguments, __LINE__);
    fb_test_readonly_check(vtab != NULL && message != NULL, __LINE__);
    for (int index = 0; index < count; index++)
        fb_test_readonly_check(arguments[index] == expected_arguments[index], __LINE__);
    *vtab = (struct sqlite3_vtab *)&vtab_tag;
    *message = result_message;
    constructor_calls++;
    return expected_result;
}

int main(void)
{
    unsigned long long record_size = 0, field_offset = 0;
    fb_test_readonly_init_layout(&record_size, &field_offset);
    fb_test_readonly_check(record_size == sizeof(struct original_init_info)
        && field_offset == offsetof(struct original_init_info, az_init), __LINE__);
    const int counts[] = {0, 1, 5, INT_MIN, INT_MAX};
    const int results[] = {0, 1, 7, 11, INT_MIN, INT_MAX};
    for (size_t count = 0; count < sizeof(counts) / sizeof(counts[0]); count++)
        for (int null_context = 0; null_context < 2; null_context++)
            for (int null_rows = 0; null_rows < 2; null_rows++)
                for (int null_names = 0; null_names < 2; null_names++)
                    for (size_t result = 0; result < sizeof(results) / sizeof(results[0]); result++) {
                        cases++;
                        fb_test_readonly_vector_case(counts[count], null_context, null_rows, null_names, results[result]);
                    }
    constructor_callback callback = fb_test_readonly_get_constructor();
    const char *const arguments[] = {"module", "main", "table", "one", "two", "three"};
    for (int count = 0; count <= 6; count++)
        for (size_t result = 0; result < sizeof(results) / sizeof(results[0]); result++) {
            cases++;
            expected_arguments = count == 0 ? NULL : arguments;
            expected_count = count;
            expected_result = results[result];
            struct sqlite3_vtab *vtab = NULL;
            char *message = NULL;
            fb_test_readonly_check(callback(&db_tag, &auxiliary_tag, count, expected_arguments,
                &vtab, &message) == expected_result, __LINE__);
            fb_test_readonly_check(vtab == (struct sqlite3_vtab *)&vtab_tag && message == result_message, __LINE__);
        }
    fb_test_readonly_check(cases == 282 && legacy_calls == 240 && constructor_calls == 42, __LINE__);
    printf("SQLITE_READONLY_PASS cases=%lu checks=%lu legacy=%lu constructors=%lu\n",
        cases, checks, legacy_calls, constructor_calls);
    return 0;
}

/* end of sqlite-readonly-vectors.c */
