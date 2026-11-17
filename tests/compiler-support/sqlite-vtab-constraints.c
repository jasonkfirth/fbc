/*
    Project FreeBASIC SQLite constraint regression
    File sqlite-vtab-constraints.c
    Purpose Drive verbatim constraint helpers and record observable operations.
    Responsibilities Check both revisions and candidate failure paths.
    The BASIC fixture owns canonical records and controlled providers.
*/
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void fb_test_constraint_check(int condition, int line);
void fb_test_constraint_trace(int operation, uint64_t value);
void fb_test_constraint_lookup(int first, int second, int third, int offset, int fault);
#ifdef SQLITE_CONSTRAINT_WHERE
void fb_test_constraint_planner(int count, int offset, int fault, int allocation,
    int failed_call, int result, int uses_in, int retry, int free_text);
#else
void fb_test_constraint_rhs(int first, int second, int third, int offset, int constraint,
    int cached, int result, int write_value, int encoding, int fault);
#endif

static unsigned long cases[2], checks[2];
static uint64_t traces[2] = {UINT64_C(14695981039346656037), UINT64_C(14695981039346656037)};
static int phase;

void fb_test_constraint_check(int condition, int line)
{
    checks[phase]++;
    if (!condition) {
        fprintf(stderr, "SQLITE_CONSTRAINT_FAIL phase=%d case=%lu line=%d\n", phase, cases[phase], line);
        exit(1);
    }
}

void fb_test_constraint_trace(int operation, uint64_t value)
{
    /* FNV-1a over fixed bytes gives a stable operation transcript across runs.
       Borrowed addresses are checked by the fixture and stay out of this hash. */
    traces[phase] ^= (unsigned int)operation;
    traces[phase] *= UINT64_C(1099511628211);
    for (unsigned int byte = 0; byte < 8; byte++) {
        traces[phase] ^= (value >> (byte * 8)) & UINT64_C(255);
        traces[phase] *= UINT64_C(1099511628211);
    }
}

static void lookup_cases(void)
{
    const int offsets[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, INT_MAX};
    for (int first = 0; first <= 4; first++)
        for (int second = 0; second <= 4; second++)
            for (int third = 0; third <= 4; third++)
                for (size_t offset = 0; offset < sizeof(offsets) / sizeof(offsets[0]); offset++) {
                    cases[phase]++;
                    fb_test_constraint_lookup(first, second, third, offsets[offset], 0);
                }
}

#ifdef SQLITE_CONSTRAINT_WHERE
static void consumer_cases(int fault_count)
{
    const int results[] = {0, 1, 7, 11};
    for (int count = 1; count <= 4; count++)
        for (int offset = 0; offset < 4; offset++)
            for (int fault = phase == 0 ? 0 : 1; fault < fault_count; fault++)
                for (int allocation = 0; allocation < 3; allocation++)
                    for (int failed_call = 0; failed_call < 5; failed_call++)
                        for (size_t result = 0; result < sizeof(results) / sizeof(results[0]); result++)
                            for (int uses_in = 0; uses_in < 2; uses_in++)
                                for (int retry = 0; retry < 2; retry++)
                                    for (int free_text = 0; free_text < 2; free_text++) {
                                        cases[phase]++;
                                        fb_test_constraint_planner(count, offset, fault, allocation,
                                            failed_call, results[result], uses_in, retry, free_text);
                                    }
}
#else
static void consumer_cases(int fault_count)
{
    const int layouts[][3] = {{1, 0, 0}, {0, 0, 1}, {2, 1, 2}, {0, 2, 2}, {4, 4, 4}};
    const int results[] = {0, 1, 7, 12};
    for (size_t layout = 0; layout < sizeof(layouts) / sizeof(layouts[0]); layout++)
        for (int offset = 0; offset < layouts[layout][0] + layouts[layout][1] + layouts[layout][2]; offset++)
            for (int constraint = 0; constraint < 4; constraint++)
                for (int fault = phase == 0 ? 0 : 1; fault < fault_count; fault++)
                    for (int cached = 0; cached < 2; cached++)
                        for (size_t result = 0; result < sizeof(results) / sizeof(results[0]); result++)
                            for (int write_value = 0; write_value < 2; write_value++)
                                for (int encoding = 1; encoding <= 3; encoding++) {
                                    cases[phase]++;
                                    fb_test_constraint_rhs(layouts[layout][0], layouts[layout][1], layouts[layout][2],
                                        offset, constraint, cached, results[result], write_value, encoding, fault);
                                }
}
#endif

int main(void)
{
    lookup_cases();
    consumer_cases(1);
#ifndef SQLITE_TEST_BEFORE
    phase = 1;
    const int negative_offsets[] = {-1, -2, INT_MIN};
    for (size_t offset = 0; offset < sizeof(negative_offsets) / sizeof(negative_offsets[0]); offset++) {
        cases[phase]++;
        fb_test_constraint_lookup(2, 2, 2, negative_offsets[offset], 0);
    }
    for (int fault = 1; fault <= 3; fault++)
        for (int offset = 0; offset < 7; offset++) {
            cases[phase]++;
            fb_test_constraint_lookup(2, 2, 2, offset, fault);
        }
#ifdef SQLITE_CONSTRAINT_WHERE
    consumer_cases(5);
#else
    consumer_cases(13);
#endif
#endif
    printf("SQLITE_CONSTRAINT_PASS common_cases=%lu common_checks=%lu common_hash=%" PRIu64
        " extra_cases=%lu extra_checks=%lu extra_hash=%" PRIu64 "\n",
        cases[0], checks[0], traces[0], cases[1], checks[1], traces[1]);
    return 0;
}

/* end of sqlite-vtab-constraints.c */
