/*
 * Project FreeBASIC Coreutils format qualification
 * File coreutils-forwarded-formats.c
 * Purpose Observe the original copy verbose-output helper.
 * Responsibilities Check forwarded formats, provider slots and output bytes.
 * Native quoting implementation and filesystem operations retain their owners.
 */
#define _GNU_SOURCE 1
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void c_audit_forwarded_copy(const char *, const char *, const char *, const char *);
char *quotearg_n_style(int, int, const char *);
char *quotearg_style(int, const char *);
char *dcgettext(const char *, const char *, int);
int __wrap_printf(const char *, ...);
int __wrap_putchar_unlocked(int);
int __real_vsnprintf(char *, size_t, const char *, va_list);

static unsigned long cases, checks, formats, backups, lines;
static const char *expected_names[3], *expected_format, *backup_format;
static unsigned int quote_slots;
static int phase;
static char quoted[3][512], translated[256], output[4096];
static size_t output_length;

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "FORWARDED_FORMAT_FAIL case=%lu check=%lu line=%d phase=%d\n",
            cases, checks, line, phase);
        exit(1);
    }
}

#define require(condition) require_at((condition), __LINE__)

/* Distinct buffers model the independent numbered quoting slots. Their call
   order is unspecified, so the observer checks a slot mask at the printf call. */
char *quotearg_n_style(int slot, int style, const char *name)
{
    require(phase == 0 && (slot == 0 || slot == 1) && style == 4);
    require(name == expected_names[slot] && !(quote_slots & (1u << slot)));
    int length = snprintf(quoted[slot], sizeof(quoted[slot]), slot ? "[destination:%s]" : "[source:%s]", name);
    require(length >= 0 && (size_t)length < sizeof(quoted[slot]));
    quote_slots |= 1u << slot;
    return quoted[slot];
}

char *quotearg_style(int style, const char *name)
{
    require(phase == 1 && style == 4 && name == expected_names[2]);
    int length = snprintf(quoted[2], sizeof(quoted[2]), "[backup:%s]", name);
    require(length >= 0 && (size_t)length < sizeof(quoted[2]));
    return quoted[2];
}

char *dcgettext(const char *domain, const char *message, int category)
{
    require(phase == 1 && domain == NULL && category == 5);
    require(strcmp(message, " (backup: %s)") == 0 && strlen(backup_format) < sizeof(translated));
    strcpy(translated, backup_format);
    return translated;
}

int __wrap_printf(const char *format, ...)
{
    va_list arguments, checked;
    va_start(arguments, format);
    va_copy(checked, arguments);
    if (phase == 0) {
        require(format == expected_format && quote_slots == 3);
        require(va_arg(checked, const void *) == quoted[0]);
        require(va_arg(checked, const void *) == quoted[1]);
    } else {
        require(phase == 1 && expected_names[2] != NULL && format == translated);
        require(va_arg(checked, const void *) == quoted[2]);
        backups++;
    }
    int length = __real_vsnprintf(output + output_length, sizeof(output) - output_length, format, arguments);
    require(length >= 0 && (size_t)length < sizeof(output) - output_length);
    output_length += (size_t)length;
    va_end(checked);
    va_end(arguments);
    formats++;
    phase++;
    return length;
}

int __wrap_putchar_unlocked(int value)
{
    require(value == '\n' && phase == (expected_names[2] ? 2 : 1));
    require(output_length + 1 < sizeof(output));
    output[output_length++] = '\n';
    output[output_length] = 0;
    lines++;
    return value;
}

static void verbose_case(const char *format, const char *source, const char *destination,
    const char *backup, int translated_backup)
{
    char reference[4096], reference_names[3][512], actual_format[256];
    int length;
    cases++;
    expected_names[0] = source;
    expected_names[1] = destination;
    expected_names[2] = backup;
    require(strlen(format) < sizeof(actual_format));
    strcpy(actual_format, format);
    expected_format = actual_format;
    backup_format = translated_backup ? " (saved [%s])" : " (backup: %s)";
    snprintf(reference_names[0], sizeof(reference_names[0]), "[source:%s]", source);
    snprintf(reference_names[1], sizeof(reference_names[1]), "[destination:%s]", destination);
    length = snprintf(reference, sizeof(reference), format, reference_names[0], reference_names[1]);
    require(length >= 0 && (size_t)length < sizeof(reference));
    size_t used = (size_t)length;
    if (backup) {
        snprintf(reference_names[2], sizeof(reference_names[2]), "[backup:%s]", backup);
        length = snprintf(reference + used, sizeof(reference) - used, backup_format, reference_names[2]);
        require(length >= 0 && (size_t)length < sizeof(reference) - used);
        used += (size_t)length;
    }
    require(used + 1 < sizeof(reference));
    reference[used++] = '\n';
    reference[used] = 0;
    phase = 0;
    quote_slots = 0;
    output_length = 0;
    output[0] = 0;
    c_audit_forwarded_copy(actual_format, source, destination, backup);
    require(output_length == used && strcmp(output, reference) == 0);
}

int main(void)
{
    static const char *const formats_text[] = {"%s -> %s", "exchanged %s <-> %s",
        "renamed %s -> %s", "copied %s -> %s", "%2$s <- %1$s",
        "exchange [%2$s] [%1$s]", "rename [%2$s] [%1$s]", "copy [%2$s] [%1$s]"};
    static const char *const names[] = {"", "a", "space name", "quote'\"", "back\\slash",
        "100% complete", "\trow\n", "\xc3\xa9.txt"};
    for (size_t format = 0; format < sizeof(formats_text) / sizeof(formats_text[0]); format++)
        for (size_t source = 0; source < sizeof(names) / sizeof(names[0]); source++)
            for (size_t destination = 0; destination < sizeof(names) / sizeof(names[0]); destination++)
                for (size_t backup = 0; backup <= sizeof(names) / sizeof(names[0]); backup++)
                    verbose_case(formats_text[format], names[source], names[destination],
                        backup ? names[backup - 1] : NULL, (int)(format % 2));
    require(lines == cases && formats == cases + backups);
    char report[256];
    snprintf(report, sizeof(report), "FORWARDED_FORMAT_PASS cases=%lu checks=%lu formats=%lu backups=%lu lines=%lu\n",
        cases, checks, formats, backups, lines);
    fputs(report, stdout);
    return 0;
}

/* end of coreutils-forwarded-formats.c */
