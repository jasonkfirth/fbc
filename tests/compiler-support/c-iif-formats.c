/*
 * Project: FreeBASIC C backend regression
 * File: c-iif-formats.c
 * Purpose: Check conditional formats against native values and call counts.
 * Responsibilities: Observe the selected translation and boundary outputs.
 * The BASIC fixture owns conditional evaluation and native format diagnostics.
 */
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int probe_branch_select(int);
const char *probe_branch_input(void);
char *dcgettext(const char *, const char *, int);
char *ngettext(const char *, const char *, unsigned long);
int probe_branch_literal(char *, size_t, int, int);
int probe_branch_translated(char *, size_t, int, int);
int probe_branch_unused(char *, size_t, int, int);
int probe_branch_plural(char *, size_t, int, int, unsigned long);
int probe_branch_cases(void);

static int selector_calls;
static int translation_calls;
static int left_calls;
static int right_calls;
static char left_text[] = "translated left %d";
static char right_text[] = "translated right %d";
static char left_many[] = "translated left many %d";
static char right_many[] = "translated right many %d";

int probe_branch_select(int choice)
{
    selector_calls++;
    return choice;
}

const char *probe_branch_input(void)
{
    return "dynamic %d";
}

char *dcgettext(const char *domain, const char *text, int category)
{
    (void)domain;
    (void)category;
    translation_calls++;
    if (strncmp(text, "left", 4) == 0) {
        left_calls++;
        return left_text;
    }
    right_calls++;
    return right_text;
}

char *ngettext(const char *one, const char *many, unsigned long count)
{
    int left = strncmp(one, "left", 4) == 0;
    (void)many;
    translation_calls++;
    if (left) {
        left_calls++;
        return count == 1 ? left_text : left_many;
    }
    right_calls++;
    return count == 1 ? right_text : right_many;
}

int probe_branch_cases(void)
{
    static const int values[] = {INT_MIN, -1, 0, 1, INT_MAX};
    static const int choices[] = {0, 1, -1};
    static const unsigned long counts[] = {0, 1, 2, ULONG_MAX};
    static const size_t capacities[] = {0, 1, 8, 64};
    size_t value_index, choice_index, count_index, capacity_index;
    char actual[64], expected[64];
    for (value_index = 0; value_index < sizeof(values) / sizeof(values[0]); value_index++) {
        for (choice_index = 0; choice_index < sizeof(choices) / sizeof(choices[0]); choice_index++) {
            int choice = choices[choice_index];
            for (capacity_index = 0; capacity_index < sizeof(capacities) / sizeof(capacities[0]); capacity_index++) {
                size_t capacity = capacities[capacity_index];
                for (count_index = 0; count_index < sizeof(counts) / sizeof(counts[0]); count_index++) {
                    unsigned long count = counts[count_index];
                    int kind;
                    for (kind = 0; kind < 4; kind++) {
                        int written, reference;
                        memset(actual, 0x5A, sizeof(actual));
                        memset(expected, 0x5A, sizeof(expected));
                        selector_calls = translation_calls = left_calls = right_calls = 0;
                        if (kind == 0) {
                            written = probe_branch_literal(actual, capacity, values[value_index], choice);
                            reference = snprintf(expected, capacity, choice ? "left %d" : "right %d", values[value_index]);
                        } else if (kind == 1) {
                            written = probe_branch_translated(actual, capacity, values[value_index], choice);
                            reference = snprintf(expected, capacity, choice ? "translated left %d" : "translated right %d", values[value_index]);
                        } else if (kind == 2) {
                            written = probe_branch_plural(actual, capacity, values[value_index], choice, count);
                            reference = snprintf(expected, capacity,
                                choice ? (count == 1 ? "translated left %d" : "translated left many %d") :
                                         (count == 1 ? "translated right %d" : "translated right many %d"), values[value_index]);
                        } else {
                            written = probe_branch_unused(actual, capacity, values[value_index], choice);
                            reference = snprintf(expected, capacity, choice ? "value %d" : "done", values[value_index]);
                        }
                        if (written != reference || memcmp(actual, expected, sizeof(actual)) != 0)
                            return 0;
                        if (selector_calls != 1 || translation_calls != (kind == 1 || kind == 2))
                            return 0;
                        if (left_calls != ((kind == 1 || kind == 2) && choice != 0) || right_calls != ((kind == 1 || kind == 2) && choice == 0))
                            return 0;
                    }
                }
            }
        }
    }
    return -1;
}

/* end of c-iif-formats.c */
