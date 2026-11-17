/*
    Project: FreeBASIC SQLite virtual-table regression
    File: sqlite-vtab-transaction.c
    Purpose: Drive canonical BASIC records through production transaction helpers.
    Responsibilities: Vary failure paths, list positions, capacity and references.
    The BASIC fixture owns records and the controlled moving allocator.
*/
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fb_test_vtab_case(int, int, int, int, int, int);
void fb_test_vtab_check(int, int);
char *sqlite3MPrintf(void *, const char *, ...);

static unsigned long cases, checks, messages;

void fb_test_vtab_check(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "SQLITE_VTAB_FAIL case=%lu check=%lu line=%d\n", cases, checks, line);
        exit(1);
    }
}

char *sqlite3MPrintf(void *db, const char *format, ...)
{
    static char message[] = "missing module";
    va_list arguments;
    va_start(arguments, format);
    fb_test_vtab_check(db != NULL && strcmp(format, "no such module: %s") == 0, __LINE__);
    fb_test_vtab_check(strcmp(va_arg(arguments, const char *), "module") == 0, __LINE__);
    va_end(arguments);
    messages++;
    return message;
}

int main(void)
{
    const int results[] = {0, 1, 7, 11};
    const int references[] = {0, 1, 17, INT_MAX - 1};
    for (int module = 0; module < 4; module++)
        for (size_t result = 0; result < sizeof(results) / sizeof(results[0]); result++)
            for (int instance = 0; instance < 4; instance++)
                for (int failure = 0; failure < 2; failure++)
                    for (int count = 0; count < 15; count++)
                        for (size_t reference = 0; reference < sizeof(references) / sizeof(references[0]); reference++) {
                            cases++;
                            fb_test_vtab_case(module, results[result], instance, failure,
                                count, references[reference]);
                        }
    fb_test_vtab_check(cases == 7680 && messages == 5760, __LINE__);
    printf("SQLITE_VTAB_PASS cases=%lu checks=%lu messages=%lu\n", cases, checks, messages);
    return 0;
}

/* end of sqlite-vtab-transaction.c */
