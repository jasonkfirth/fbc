/*
    Project: FreeBASIC SQLite error-path regression
    File: sqlite-vdbe-error-context.c
    Purpose: Observe the real helper's callback order and connection state.
    Responsibilities: Compare return codes, pointer identity and corruption flags.
    The BASIC fixture owns canonical database, VM and instruction records.
*/
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void fb_test_sqlite_set_autocommit(void *, int);
void fb_test_sqlite_error_case(int, int, int, int, int, int, int,
    unsigned long long, unsigned long long *);
void fb_test_sqlite_prepare(void *, void *, void *, void *, int, int, int, int);
void fb_test_sqlite_system_error(void *, int);
void fb_test_sqlite_log_abort(void *, int, void *, void *);
int fb_test_sqlite_finish(void *, int, int);
void fb_test_sqlite_leave(void *);

/* SQLite's internal error helper uses CORRUPT=11, DONE=101 and OE_Abort=2.
   Bit 33 records a corruption failure inside an explicit transaction. */
enum { probe_corrupt = 11, probe_done = 101, probe_abort = 2 };
static const unsigned long long corrupt_readonly = 1ull << 33;

/* The serial observer borrows these addresses only during one helper call. */
static void *expected_vm, *expected_db, *expected_operation, *expected_operations;
static int expected_result, finish_result, final_autocommit, expected_action;
static unsigned trace;
static unsigned long cases, checks;

static void check(int condition)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "SQLITE_VDBE_ERROR_FAIL case=%lu check=%lu trace=%u\n", cases, checks, trace);
        exit(1);
    }
}

void fb_test_sqlite_prepare(void *vm, void *db, void *operation, void *operations,
    int result, int finish, int after_autocommit, int action)
{
    expected_vm = vm;
    expected_db = db;
    expected_operation = operation;
    expected_operations = operations;
    expected_result = result;
    finish_result = finish;
    final_autocommit = after_autocommit;
    expected_action = action == 0 ? probe_abort : action;
    trace = 0;
}

void fb_test_sqlite_system_error(void *db, int result)
{
    check(trace == 0 && db == expected_db && result == expected_result);
    trace = 1;
}

void fb_test_sqlite_log_abort(void *vm, int result, void *operation, void *operations)
{
    check(trace == 1 && vm == expected_vm && result == expected_result);
    check(operation == expected_operation && operations == expected_operations);
    trace = 12;
}

int fb_test_sqlite_finish(void *vm, int result, int action)
{
    check(trace == 12 && vm == expected_vm && result == expected_result);
    check(action == expected_action);
    fb_test_sqlite_set_autocommit(expected_db, final_autocommit);
    trace = 123;
    return finish_result;
}

void fb_test_sqlite_leave(void *vm)
{
    check(trace == 123 && vm == expected_vm);
    trace = 1234;
}

int main(void)
{
    static const int results[] = {-1, 0, 1, 5, 7, probe_corrupt, 267, INT_MAX};
    static const int finishes[] = {0, 1, 5, probe_done};
    static const int actions[] = {0, 1, probe_abort, 3, 255};
    static const unsigned long long flags[] = {0, 256, 1ull << 33, ULLONG_MAX};
    for (int has_vm = 0; has_vm <= 1; has_vm++)
        for (int has_db = 0; has_db <= 1; has_db++)
            for (size_t r = 0; r < sizeof(results) / sizeof(results[0]); r++)
                for (size_t f = 0; f < sizeof(finishes) / sizeof(finishes[0]); f++)
                    for (size_t a = 0; a < sizeof(actions) / sizeof(actions[0]); a++)
                        for (size_t k = 0; k < sizeof(flags) / sizeof(flags[0]); k++)
                            for (int initial = 0; initial <= 2; initial++)
                                for (int after = 0; after <= 2; after++) {
                                    unsigned long long observed[4] = {0};
                                    cases++;
                                    fb_test_sqlite_error_case(has_vm, has_db, results[r], initial,
                                        after, actions[a], finishes[f], flags[k], observed);
                                    int active = has_vm && has_db;
                                    int returned = active ? (finishes[f] == probe_done ? probe_done : 1) : results[r];
                                    unsigned long long expected_flags = flags[k];
                                    if (active && results[r] == probe_corrupt && after == 0)
                                        expected_flags |= corrupt_readonly;
                                    check(observed[0] == (uint32_t)returned);
                                    check(observed[1] == expected_flags);
                                    check(observed[2] == (unsigned)(active ? after : initial));
                                    check(observed[3] == (unsigned)actions[a]);
                                    check(trace == (active ? 1234u : 0u));
                                }
    printf("SQLITE_VDBE_ERROR_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}

/* end of sqlite-vdbe-error-context.c */
