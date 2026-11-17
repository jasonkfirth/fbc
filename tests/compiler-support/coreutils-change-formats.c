/*
 * Project FreeBASIC Coreutils format qualification
 * File coreutils-change-formats.c
 * Purpose Compare complete chmod/chown reporting functions with original C.
 * Responsibilities Check format arguments, translation, output and ownership.
 * Filesystem changes and the real gettext/quoting providers have separate tests.
 */
#define _GNU_SOURCE 1
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void audit_actual_chmod(const char *, int, unsigned int, unsigned int);
void audit_reference_chmod(const char *, int, unsigned int, unsigned int);
void audit_actual_chown(const char *, int, const char *, const char *, const char *, const char *);
void audit_reference_chown(const char *, int, const char *, const char *, const char *, const char *);
char *dcgettext(const char *, const char *, int);
char *quotearg_style(int, const char *);
void strmode(unsigned int, char *);
void *xmalloc(size_t);
char *xstrdup(const char *);
int __wrap_printf(const char *, ...);
void __wrap_free(void *);
void __real_free(void *);
void __wrap___assert_fail(const char *, const char *, unsigned int, const char *) __attribute__((noreturn));

struct observation {
    char output[2048];
    char format[256];
    unsigned int prints, quotes, translations, modes, allocations, frees, asserted;
};

static struct observation current;
static jmp_buf assertion_target;
static void *owned[4];
static char quote_buffer[512], translated[256];
static const char *expected_file;
static unsigned long cases, checks;
static int translation_mode;

static void require_at(int condition, int line)
{
    checks++;
    if (!condition) {
        fprintf(stderr, "CHANGE_FORMAT_FAIL case=%lu check=%lu line=%d\n", cases, checks, line);
        exit(1);
    }
}

#define require(condition) require_at((condition), __LINE__)

/* Translation storage and quoting storage have independent lifetimes. The
   positional mode retains every conversion's original type and position. */
char *dcgettext(const char *domain, const char *format, int category)
{
    /* gettext's native declaration requires a nonnull message identifier. */
    require(domain == NULL && category == 5);
    current.translations++;
    size_t output = 0;
    if (translation_mode) {
        memcpy(translated, "translated ", 11);
        output = 11;
    }
    unsigned int argument = 0;
    for (size_t input = 0; format[input]; input++) {
        require(output + 8 < sizeof(translated));
        translated[output++] = format[input];
        if (format[input] == '%' && translation_mode == 2) {
            require(argument < 5);
            argument++;
            translated[output++] = (char)('0' + argument);
            translated[output++] = '$';
        }
    }
    translated[output] = '\0';
    return translated;
}

char *quotearg_style(int style, const char *file)
{
    require(style == 4 && file == expected_file);
    current.quotes++;
    int size = snprintf(quote_buffer, sizeof(quote_buffer), "[%s]", file);
    require(size >= 0 && (size_t)size < sizeof(quote_buffer));
    return quote_buffer;
}

void strmode(unsigned int mode, char *buffer)
{
    /* Both implementations must use bytes 1..9 and remove byte 10. Distinct
       deterministic modes make swapped old/new arguments observable. */
    current.modes++;
    buffer[0] = '-';
    for (unsigned int index = 0; index < 9; index++)
        buffer[index + 1] = (mode & (1u << index)) ? 'x' : '-';
    buffer[10] = ' ';
    buffer[11] = '\0';
}

void *xmalloc(size_t bytes)
{
    require(bytes > 0 && bytes < 1024);
    void *result = malloc(bytes);
    require(result != NULL);
    for (unsigned int index = 0; index < 4; index++) {
        if (owned[index] == NULL) {
            owned[index] = result;
            current.allocations++;
            return result;
        }
    }
    require(0);
    return NULL;
}

char *xstrdup(const char *text)
{
    size_t bytes = strlen(text) + 1;
    char *result = xmalloc(bytes);
    memcpy(result, text, bytes);
    return result;
}

void __wrap_free(void *address)
{
    if (address == NULL)
        return;
    for (unsigned int index = 0; index < 4; index++) {
        if (owned[index] == address) {
            owned[index] = NULL;
            current.frees++;
            __real_free(address);
            return;
        }
    }
    require(0);
}

int __wrap_printf(const char *format, ...)
{
    require(format != NULL && current.prints == 0);
    current.prints++;
    size_t bytes = strlen(format) + 1;
    require(bytes <= sizeof(current.format));
    memcpy(current.format, format, bytes);
    va_list arguments;
    va_start(arguments, format);
    int size = vsnprintf(current.output, sizeof(current.output), format, arguments);
    va_end(arguments);
    require(size >= 0 && (size_t)size < sizeof(current.output));
    return size;
}

void __wrap___assert_fail(const char *expression, const char *file, unsigned int line, const char *function)
{
    require(expression != NULL && file != NULL && line > 0);
    require(strcmp(function, "describe_change") == 0);
    current.asserted++;
    longjmp(assertion_target, 1);
}

static void begin_observation(const char *file, int translation)
{
    memset(&current, 0, sizeof(current));
    for (unsigned int index = 0; index < 4; index++)
        require(owned[index] == NULL);
    expected_file = file;
    translation_mode = translation;
}

static struct observation end_observation(int valid)
{
    require(current.asserted == (unsigned int)!valid);
    require(current.prints == (unsigned int)valid);
    require(current.translations == (unsigned int)valid);
    if (valid)
        require(current.allocations == current.frees);
    /* Invalid enum values end in a nonreturning assertion in the application.
       The observer resumes there and releases only its own tracked storage. */
    for (unsigned int index = 0; index < 4; index++) {
        __real_free(owned[index]);
        owned[index] = NULL;
    }
    return current;
}

static void compare(struct observation reference, struct observation actual)
{
    require(strcmp(reference.output, actual.output) == 0);
    require(strcmp(reference.format, actual.format) == 0);
    require(reference.prints == actual.prints);
    require(reference.quotes == actual.quotes);
    require(reference.translations == actual.translations);
    require(reference.modes == actual.modes);
    require(reference.allocations == actual.allocations);
    require(reference.frees == actual.frees);
    require(reference.asserted == actual.asserted);
    cases++;
}

static struct observation observe_chmod(int actual, const char *file, int status,
    unsigned int old_mode, unsigned int new_mode, int translation)
{
    begin_observation(file, translation);
    if (setjmp(assertion_target) == 0) {
        if (actual)
            audit_actual_chmod(file, status, old_mode, new_mode);
        else
            audit_reference_chmod(file, status, old_mode, new_mode);
    }
    return end_observation(status >= 0 && status <= 4);
}

static struct observation observe_chown(int actual, const char *file, int status,
    const char *old_user, const char *old_group, const char *user, const char *group, int translation)
{
    begin_observation(file, translation);
    if (setjmp(assertion_target) == 0) {
        if (actual)
            audit_actual_chown(file, status, old_user, old_group, user, group);
        else
            audit_reference_chown(file, status, old_user, old_group, user, group);
    }
    return end_observation(status >= 1 && status <= 4);
}

int main(void)
{
    static const char *const files[] = {"file", "space and %s", "", "quote'and\nnewline"};
    static const unsigned int modes[] = {0, 1, 0777, 07000, 07777, UINT_MAX};
    static const int statuses[] = {INT_MIN, -1, 0, 1, 2, 3, 4, 5, INT_MAX};
    static const char *const users[] = {NULL, "", "owner%s"};
    static const char *const groups[] = {NULL, "", "group%lo"};
    for (unsigned int file = 0; file < sizeof(files) / sizeof(files[0]); file++) {
        for (int translation = 0; translation < 3; translation++) {
            for (unsigned int status = 0; status < sizeof(statuses) / sizeof(statuses[0]); status++) {
                for (unsigned int old_mode = 0; old_mode < sizeof(modes) / sizeof(modes[0]); old_mode++) {
                    for (unsigned int new_mode = 0; new_mode < sizeof(modes) / sizeof(modes[0]); new_mode++) {
                        compare(observe_chmod(0, files[file], statuses[status], modes[old_mode], modes[new_mode], translation),
                            observe_chmod(1, files[file], statuses[status], modes[old_mode], modes[new_mode], translation));
                    }
                }
                for (unsigned int old_user = 0; old_user < 3; old_user++) {
                    for (unsigned int old_group = 0; old_group < 3; old_group++) {
                        for (unsigned int user = 0; user < 3; user++) {
                            for (unsigned int group = 0; group < 3; group++) {
                                /* Successful and retained messages consume old_spec when a
                                   new owner/group is selected. Keep that caller contract. */
                                int known_old = (user && old_user) || (group && old_group);
                                if ((statuses[status] == 2 || statuses[status] == 4) &&
                                    (user || group) && !known_old)
                                    continue;
                                compare(observe_chown(0, files[file], statuses[status], users[old_user], groups[old_group], users[user], groups[group], translation),
                                    observe_chown(1, files[file], statuses[status], users[old_user], groups[old_group], users[user], groups[group], translation));
                            }
                        }
                    }
                }
            }
        }
    }
    fprintf(stdout, "CHANGE_FORMAT_PASS cases=%lu checks=%lu\n", cases, checks);
    return 0;
}
/* end of coreutils-change-formats.c */
