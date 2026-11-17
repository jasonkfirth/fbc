/*
    Project: FreeBASIC C backend regression
    File: c-format-locals.c
    Purpose: Compare actual local-format output with native snprintf.
    Responsibilities: Check results, pointer identity and observation count.
    The paired BASIC fixture owns pointer bindings and compiler diagnostics.
*/

#include <stddef.h>
#include <stdio.h>
#include <string.h>

int probe_local_format_direct(char *, size_t, int);
int probe_local_format_across_call(char *, size_t, int);
int probe_local_format_reassign(char *, size_t, int);
int probe_local_format_sprintf(char *, int);
int probe_local_format_identity(char *, size_t, int);
const char *probe_local_format_foreign(char *, size_t, int, int);
const char *fb_FormatValue(const char *, const char *);
const char *fb_c_format_value(const char *, const char *);
int probe_local_format_observe(const char *, const char *);
void probe_local_format_barrier(void);
int probe_local_format_argument(int);
void probe_local_format_replace(const char **);
const char *probe_local_format_input(void);
int probe_local_format_cases(void);
int probe_local_format_translated(char *, size_t, int);
int probe_local_format_translated_plural(char *, size_t, int, unsigned long);
char *gettext(const char *);
char *ngettext(const char *, const char *, unsigned long);
int probe_local_translation_observe(const char *);
void probe_local_translation_mutate(void);

static int observations;
static int argument_calls;
static int foreign_legacy_calls;
static int foreign_reserved_calls;
static const char foreign_legacy_text[] = "foreign legacy";
static const char foreign_reserved_text[] = "foreign reserved";
/* Translation routines retain this storage across calls. Its address differs
   from every source literal, so substituting the source format fails the test. */
static char translation_text[64];
static int translation_calls;
static int translation_observations;

char *gettext(const char *text)
{
    if (strcmp(text, "source %d") != 0)
        return NULL;
    translation_calls++;
    strcpy(translation_text, "translated %d");
    return translation_text;
}

char *ngettext(const char *one, const char *many, unsigned long count)
{
    if (strcmp(one, "one %d") != 0 || strcmp(many, "many %d") != 0)
        return NULL;
    translation_calls++;
    strcpy(translation_text, count == 1 ? "translated one %d" : "translated many %d");
    return translation_text;
}

int probe_local_translation_observe(const char *text)
{
    translation_observations++;
    return text == translation_text;
}

void probe_local_translation_mutate(void)
{
    strcpy(translation_text, "changed %s");
}

const char *fb_FormatValue(const char *a, const char *b)
{
    (void)a;
    (void)b;
    foreign_legacy_calls++;
    return foreign_legacy_text;
}

const char *fb_c_format_value(const char *a, const char *b)
{
    (void)a;
    (void)b;
    foreign_reserved_calls++;
    return foreign_reserved_text;
}

int probe_local_format_observe(const char *actual, const char *expected)
{
    observations++;
    return actual == expected;
}

void probe_local_format_barrier(void)
{
    observations++;
}

int probe_local_format_argument(int value)
{
    argument_calls++;
    return value;
}

void probe_local_format_replace(const char **value)
{
    *value = "%d";
}

const char *probe_local_format_input(void)
{
    return "%d";
}

int probe_local_format_cases(void)
{
    static const int values[] = {-2147483647 - 1, -1, 0, 1, 2147483647};
    char actual[64];
    char expected[64];
    size_t position;
    int actual_length;
    int expected_length;

    observations = 0;
    argument_calls = 0;
    foreign_legacy_calls = 0;
    foreign_reserved_calls = 0;
    translation_calls = 0;
    translation_observations = 0;
    for (position = 0; position < sizeof(values) / sizeof(values[0]); position++) {
        expected_length = snprintf(expected, sizeof(expected), "%d", values[position]);
        actual_length = probe_local_format_direct(actual, sizeof(actual), values[position]);
        if (actual_length != expected_length || strcmp(actual, expected) != 0)
            return 0;
        expected_length = snprintf(expected, sizeof(expected), "translated %d", values[position]);
        actual_length = probe_local_format_translated(actual, sizeof(actual), values[position]);
        if (actual_length != expected_length || strcmp(actual, expected) != 0)
            return 0;
        for (unsigned long count = 0; count < 3; count++) {
            expected_length = snprintf(expected, sizeof(expected),
                count == 1 ? "translated one %d" : "translated many %d", values[position]);
            actual_length = probe_local_format_translated_plural(actual, sizeof(actual), values[position], count);
            if (actual_length != expected_length || strcmp(actual, expected) != 0)
                return 0;
        }
        expected_length = snprintf(expected, sizeof(expected), "across %d", values[position]);
        actual_length = probe_local_format_across_call(actual, sizeof(actual), values[position]);
        if (actual_length != expected_length || strcmp(actual, expected) != 0)
            return 0;
        expected_length = snprintf(expected, sizeof(expected), "%d", values[position]);
        if (probe_local_format_foreign(actual, sizeof(actual), values[position], 0) != foreign_legacy_text ||
            strcmp(actual, expected) != 0)
            return 0;
        if (probe_local_format_foreign(actual, sizeof(actual), values[position], 1) != foreign_reserved_text ||
            strcmp(actual, expected) != 0)
            return 0;

        expected_length = snprintf(expected, sizeof(expected), "value %d", values[position]);
        actual_length = probe_local_format_reassign(actual, sizeof(actual), values[position]);
        if (actual_length != expected_length || strcmp(actual, expected) != 0)
            return 0;

        expected_length = snprintf(expected, sizeof(expected), "sprintf %d", values[position]);
        actual_length = probe_local_format_sprintf(actual, values[position]);
        if (actual_length != expected_length || strcmp(actual, expected) != 0)
            return 0;

        expected_length = snprintf(expected, sizeof(expected), "identity %d", values[position]);
        actual_length = probe_local_format_identity(actual, sizeof(actual), values[position]);
        if (actual_length != expected_length || strcmp(actual, expected) != 0)
            return 0;
    }
    return observations == 10 && argument_calls == 5 &&
        foreign_legacy_calls == 5 && foreign_reserved_calls == 5 &&
        translation_calls == 20 && translation_observations == 20 ? -1 : 0;
}

/* end of c-format-locals.c */
