/*
 * Project: FreeBASIC C backend regression
 * File: c-literal-offsets.c
 * Purpose: Observe literal suffix addresses and source condition calls.
 * Responsibilities: Count calls and compare two addresses in the same literal.
 * Format diagnostics belong to the BASIC fixture and diagnostic driver.
 */
#include <string.h>
#include <stdint.h>

typedef const char *(*literal_offset_picker)(int);
int probe_literal_offset_argument(int value);
void probe_literal_offset_reset(void);
int probe_literal_offset_calls(void);
int probe_literal_offset_observe(literal_offset_picker picker);

static int calls;

int probe_literal_offset_argument(int value)
{
    ++calls;
    return value;
}

void probe_literal_offset_reset(void)
{
    calls = 0;
}

int probe_literal_offset_calls(void)
{
    return calls;
}

int probe_literal_offset_observe(literal_offset_picker picker)
{
    const char *base = picker(1);
    const char *suffix = picker(0);
    return (uintptr_t)suffix == (uintptr_t)base + 1u && strcmp(base, " %d") == 0 && strcmp(suffix, "%d") == 0 ? -1 : 0;
}

#ifdef LITERAL_OFFSET_C_MAIN
#include <limits.h>
#include <stdio.h>

const char *probe_literal_offset_pick(int condition);
int probe_literal_offset_write(char *destination, size_t capacity, int condition, int mode);

int main(void)
{
    static const int conditions[] = {INT_MIN, -1, 0, 1, INT_MAX};
    char destination[64];
    for (size_t position = 0; position < sizeof(conditions) / sizeof(conditions[0]); ++position) {
        int condition = conditions[position];
        const char *expected = condition == 0 ? "23" : " 23";
        for (int mode = 0; mode < 3; ++mode) {
            int written;
            probe_literal_offset_reset();
            written = probe_literal_offset_write(destination, sizeof(destination), condition, mode);
            if (written != (int)strlen(expected) || strcmp(destination, expected) != 0 || calls != 1) return 1;
        }
    }
    probe_literal_offset_reset();
    if (probe_literal_offset_observe(probe_literal_offset_pick) != -1 || calls != 2) return 2;
    puts("LITERAL_OFFSETS_NATIVE_PASS");
    return 0;
}
#endif

/* end of c-literal-offsets.c */
