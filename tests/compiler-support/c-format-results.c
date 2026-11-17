/*
    Project: FreeBASIC C backend regression
    File: c-format-results.c
    Purpose: Supply deterministic native translation results and observations.
    Responsibilities: Record call ordering and verify returned pointer identity.
*/

#include <string.h>

char *gettext(const char *text);
char *dgettext(const char *domain, const char *text);
char *dcgettext(const char *domain, const char *text, int category);
void probe_format_reset(void);
int probe_format_steps(void);
int probe_format_argument(int value);
char *probe_format_result(void);
int probe_format_observe(char *value);

/* Static backing storage makes every observed result's lifetime explicit. */
static char translated[] = "translated %d";
static int steps;

char *gettext(const char *text)
{
    steps = steps * 10 + 1;
    return strcmp(text, "source %d") == 0 ? translated : 0;
}

char *dgettext(const char *domain, const char *text)
{
    steps = steps * 10 + 2;
    return strcmp(domain, "domain") == 0 && strcmp(text, "source %d") == 0 ? translated : 0;
}

char *dcgettext(const char *domain, const char *text, int category)
{
    steps = steps * 10 + 3;
    return strcmp(domain, "domain") == 0 && strcmp(text, "source %d") == 0 && category == 5 ? translated : 0;
}

void probe_format_reset(void)
{
    steps = 0;
}

int probe_format_steps(void)
{
    return steps;
}

int probe_format_argument(int value)
{
    steps = steps * 10 + 4;
    return value;
}

char *probe_format_result(void)
{
    return translated;
}

int probe_format_observe(char *value)
{
    return value == translated ? -1 : 0;
}

/* end of c-format-results.c */
