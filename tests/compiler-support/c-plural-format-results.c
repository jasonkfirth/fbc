/*
    Project: FreeBASIC C backend regression
    File: c-plural-format-results.c
    Purpose: Supply native plural translation values and independent observations.
    Responsibilities: Check C unsigned-long counts, call counts and returned storage.
    Compiler diagnostic controls belong to the accompanying Python driver.
*/

#include <limits.h>
#include <stdio.h>
#include <string.h>

char *ngettext(const char *one, const char *many, unsigned long count);
char *dngettext(const char *domain, const char *one, const char *many, unsigned long count);
char *dcngettext(const char *domain, const char *one, const char *many, unsigned long count, int category);
void probe_plural_reset(unsigned long count);
unsigned long probe_plural_count(unsigned long count);
int probe_plural_argument(void);
int probe_plural_text(const char *text, int written);
int probe_plural_identity(char *text);
int plural_format_case(int kind, unsigned long count);
int plural_identity_case(int kind, unsigned long count);

/* The result storage is independent of the source message literals. */
static char singular[] = "single %d";
static char plural[] = "plural %d";
static unsigned long expected_count;
static int translation_calls;
static int count_calls;
static int argument_calls;
static int invalid;

char *ngettext(const char *one, const char *many, unsigned long count)
{
    translation_calls++;
    invalid |= strcmp(one, "source %d") != 0 || strcmp(many, "sources %d") != 0;
    invalid |= count != expected_count;
    return count == 1 ? singular : plural;
}

char *dngettext(const char *domain, const char *one, const char *many, unsigned long count)
{
    invalid |= strcmp(domain, "domain") != 0;
    return ngettext(one, many, count);
}

char *dcngettext(const char *domain, const char *one, const char *many, unsigned long count, int category)
{
    invalid |= category != 5;
    return dngettext(domain, one, many, count);
}

void probe_plural_reset(unsigned long count)
{
    expected_count = count;
    translation_calls = 0;
    count_calls = 0;
    argument_calls = 0;
    invalid = 0;
}

unsigned long probe_plural_count(unsigned long count)
{
    count_calls++;
    return count;
}

int probe_plural_argument(void)
{
    argument_calls++;
    return 7;
}

int probe_plural_text(const char *text, int written)
{
    const char *expected = expected_count == 1 ? "single 7" : "plural 7";
    return !invalid && translation_calls == 1 && count_calls == 1 && argument_calls == 1
        && written == 8 && strcmp(text, expected) == 0;
}

int probe_plural_identity(char *text)
{
    char *expected = expected_count == 1 ? singular : plural;
    return !invalid && translation_calls == 1 && count_calls == 1 && argument_calls == 0
        && text == expected;
}

#ifdef PLURAL_FORMAT_C_MAIN
int main(void)
{
    const unsigned long counts[] = {0, 1, 2, ULONG_MAX};
    int kind;
    unsigned int i;
    for (kind = 0; kind < 3; kind++) {
        for (i = 0; i < sizeof(counts) / sizeof(counts[0]); i++) {
            if (!plural_format_case(kind, counts[i]) || !plural_identity_case(kind, counts[i]))
                return 1;
        }
    }
    puts("PLURAL_FORMAT_RESULTS_NATIVE_PASS");
    return 0;
}
#endif

/* end of c-plural-format-results.c */
