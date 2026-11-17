/*
    Project: FreeBASIC SQLite ALTER regression
    File: sqlite-drop-column.c
    Purpose: Observe DROP COLUMN output, argument types and cleanup ordering.
    Responsibilities: Drive lexical boundaries and missing-token error paths.
    The BASIC fixture owns canonical records and the original helper bodies.
*/
#include <limits.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fb_test_drop_case(const char *, const int *, int, int, int, int, int, int, int, int, int);
void fb_test_drop_check(int, int);
void fb_test_drop_prepare(void *, void *);
void fb_test_drop_cleanup(void);
int fb_test_drop_authorizer_restored(void);
char *sqlite3MPrintf(void *, const char *, ...);
int sqlite3CorruptError(int);
void sqlite3_result_text(void *, const char *, int, void *);
void sqlite3_result_error_code(void *, int);
void sqlite3_free(void *);

/* The serial case retains its SQL and expected output until every callback
   returns. SQLite error 11 denotes a corrupt schema, and error 7 is NOMEM. */
static unsigned long cases, checks, formatted, rejected;
static void *expected_db, *expected_context;
static const char *expected_sql, *expected_tail;
static int expected_precision, expected_error, cleanup_calls, text_calls, free_calls, error_calls;
static char output[2048], expected_output[2048];

void fb_test_drop_check(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "SQLITE_DROP_FAIL case=%lu check=%lu line=%d\n", cases, checks, line);
        exit(1);
    }
}
#define CHECK(condition) fb_test_drop_check((condition), __LINE__)

void fb_test_drop_prepare(void *db, void *context)
{
    CHECK(db != NULL && context != NULL);
    expected_db = db;
    expected_context = context;
    cleanup_calls = text_calls = free_calls = error_calls = 0;
}

void fb_test_drop_cleanup(void)
{
    CHECK(cleanup_calls == 0);
    CHECK(text_calls == (expected_error == 0) && free_calls == text_calls);
    cleanup_calls++;
}

char *sqlite3MPrintf(void *db, const char *format, ...)
{
    CHECK(db == expected_db && cleanup_calls == 0 && expected_error == 0);
    CHECK(strcmp(format, "%.*s%s") == 0);
    va_list arguments;
    va_start(arguments, format);
#ifdef AUDIT_DROP_BEFORE
    /* The old emitted call passes an int64 pointer difference. Observe that
       actual type without performing SQLite's incompatible int extraction. */
    long long old_precision = va_arg(arguments, long long);
    CHECK(old_precision >= 0 && old_precision < (long long)sizeof(output));
    int precision = (int)old_precision;
#else
    int precision = va_arg(arguments, int);
#endif
    const char *source = va_arg(arguments, const char *);
    const char *tail = va_arg(arguments, const char *);
    va_end(arguments);
    CHECK(precision == expected_precision && source == expected_sql && tail == expected_tail);
    int length = snprintf(output, sizeof(output), "%.*s%s", precision, source, tail);
    CHECK(length >= 0 && (size_t)length < sizeof(output));
    CHECK(strcmp(output, expected_output) == 0);
    formatted++;
    return output;
}

int sqlite3CorruptError(int line)
{
    CHECK(line > 0 && expected_error == 11 && cleanup_calls == 0);
    return 11;
}

void sqlite3_result_text(void *context, const char *text, int length, void *destructor)
{
    CHECK(context == expected_context && text == output && length == -1);
    CHECK((uintptr_t)destructor == UINTPTR_MAX && cleanup_calls == 0);
    CHECK(strcmp(text, expected_output) == 0 && text_calls == 0);
    text_calls++;
}

void sqlite3_free(void *allocation)
{
    CHECK(allocation == output && text_calls == 1 && free_calls == 0 && cleanup_calls == 0);
    free_calls++;
}

void sqlite3_result_error_code(void *context, int result)
{
    CHECK(context == expected_context && result == expected_error && expected_error != 0);
    CHECK(cleanup_calls == 1 && fb_test_drop_authorizer_restored() && error_calls == 0);
    error_calls++;
    rejected++;
}

int main(void)
{
    static const char *const definitions[][3] = {
        {"a INT", "b TEXT", "c REAL"},
        {"a DECIMAL(3,2)", "b TEXT DEFAULT 'a,b'", "c REAL"},
        {"a INT /*a,b*/", "b TEXT DEFAULT (printf('%s,%s','x','y'))", "c REAL /*end*/"},
        {"\"a\" INT", "[b] TEXT", "`c` REAL"}
    };
    static const char *const separators[] = {",", ", ", ",\n  ", ", /*next*/ "};
    static const int parse_results[] = {0, 1, 7, 11};
    static const int column_counts[] = {0, 1, 3};
    for (size_t variant = 0; variant < sizeof(definitions) / sizeof(definitions[0]); variant++) {
        char sql[2048];
        const char *prefix = "CREATE TABLE t(";
        const char *separator = separators[variant];
        const char *suffix = ")";
        int offsets[3];
        offsets[0] = (int)strlen(prefix);
        offsets[1] = offsets[0] + (int)strlen(definitions[variant][0]) + (int)strlen(separator);
        offsets[2] = offsets[1] + (int)strlen(definitions[variant][1]) + (int)strlen(separator);
        int closing = offsets[2] + (int)strlen(definitions[variant][2]);
        int length = snprintf(sql, sizeof(sql), "%s%s%s%s%s%s%s", prefix, definitions[variant][0],
            separator, definitions[variant][1], separator, definitions[variant][2], suffix);
        CHECK(length >= 0 && (size_t)length < sizeof(sql));
        for (int column = -1; column <= 3; column++)
            for (size_t count = 0; count < sizeof(column_counts) / sizeof(column_counts[0]); count++)
                for (int mask = 0; mask < 8; mask++)
                    for (int reverse = 0; reverse < 2; reverse++)
                        for (size_t parsed = 0; parsed < sizeof(parse_results) / sizeof(parse_results[0]); parsed++)
                            for (int table = 0; table < 2; table++)
                                for (int authorizer = 0; authorizer < 2; authorizer++)
                                    for (int schema = 0; schema < 2; schema++) {
#ifdef AUDIT_DROP_BEFORE
                                        /* Missing-token and negative-index cases exercise the repair.
                                           The original helper is executed only on its defined paths. */
                                        if (mask != 7 || column < 0)
                                            continue;
#endif
                                        cases++;
                                        expected_error = parse_results[parsed];
                                        if (!expected_error && (!table || column_counts[count] <= 1 ||
                                            column < 0 || column >= column_counts[count]))
                                            expected_error = 11;
                                        int needed = column == 2 ? 2 : (column >= 0 && column < 2 ? 3 << column : 0);
                                        if (!expected_error && (mask & needed) != needed)
                                            expected_error = 11;
                                        expected_sql = sql;
                                        expected_precision = 0;
                                        expected_tail = sql;
                                        if (!expected_error) {
                                            expected_precision = column == 2 ? offsets[2] - (int)strlen(separator) : offsets[column];
                                            expected_tail = sql + (column == 2 ? closing : offsets[column + 1]);
                                            length = snprintf(expected_output, sizeof(expected_output), "%.*s%s",
                                                expected_precision, sql, expected_tail);
                                            CHECK(length >= 0 && (size_t)length < sizeof(expected_output));
                                        }
                                        fb_test_drop_case(sql, offsets, closing, column, column_counts[count], mask,
                                            reverse, parse_results[parsed], table, authorizer, schema);
                                        CHECK(cleanup_calls == 1 && error_calls == (expected_error != 0));
                                    }
    }
    CHECK(cases > 0 && formatted > 0 && rejected > 0);
    printf("SQLITE_DROP_PASS cases=%lu checks=%lu formatted=%lu rejected=%lu\n", cases, checks, formatted, rejected);
    return 0;
}

/* end of sqlite-drop-column.c */
