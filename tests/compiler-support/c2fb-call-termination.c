/*
 * Project FreeBASIC C2FB qualification
 * File c2fb-call-termination.c
 * Purpose Observe conditional GNU error calls and generated terminal guards.
 * Responsibilities Supply native providers and a scalar entry for each case.
 * Source conversion and whole-module diagnostics belong to the Python runner.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__)
#include <error.h>
#endif

int audit_error_zero(void);
int audit_error_one(void);
int audit_error_negative(void);
int audit_error_expression(void);
int audit_error_variable(int status);
int audit_line_zero(void);
int audit_line_one(void);
int audit_line_variable(int status);
int audit_explicit_stop(void);
int audit_nullable_value(int *value);
void audit_error_provider(int status, int code, const char *format, ...);
void audit_error_at_line_provider(int status, int code, const char *file,
                                  unsigned int line, const char *format, ...);
_Noreturn void audit_stop(void);

/* Each executable owns one call and one provider mode. The returning mode
 * deliberately violates the documented nonzero-status contract to observe
 * the generated terminal guard without dereferencing an invalid pointer. */
static int return_from_error;

#if defined(__linux__)
static void print_program_name(void)
{
    fputs("call-contract: ", stderr);
}
#endif

void audit_error_provider(int status, int code, const char *format, ...)
{
    if (return_from_error) return;
#if defined(__linux__)
    /* Fixture formats contain plain text. The actual GNU function owns
     * reporting, zero-status return and nonzero-status process termination. */
    error(status, code, "%s", format);
#else
    (void)code;
    fprintf(stderr, "call-contract: %s\n", format);
    if (status != 0) exit(status);
#endif
}

void audit_error_at_line_provider(int status, int code, const char *file,
                                  unsigned int line, const char *format, ...)
{
    if (return_from_error) return;
#if defined(__linux__)
    error_at_line(status, code, file, line, "%s", format);
#else
    (void)code;
    fprintf(stderr, "call-contract: %s:%u: %s\n", file, line, format);
    if (status != 0) exit(status);
#endif
}

_Noreturn void audit_stop(void)
{
    exit(49);
}

int main(int argc, char **argv)
{
    int which;
    int status;
    int result;
    int value = 57;
    if (argc < 3 || argc > 4) return 90;
    which = atoi(argv[1]);
    status = atoi(argv[2]);
    return_from_error = argc == 4 && strcmp(argv[3], "violate") == 0;
#if defined(__linux__)
    error_print_progname = print_program_name;
#endif
    switch (which) {
    case 0: result = audit_error_zero(); break;
    case 1: result = audit_error_one(); break;
    case 2: result = audit_error_negative(); break;
    case 3: result = audit_error_expression(); break;
    case 4: result = audit_error_variable(status); break;
    case 5: result = audit_line_zero(); break;
    case 6: result = audit_line_one(); break;
    case 7: result = audit_line_variable(status); break;
    case 8: result = audit_explicit_stop(); break;
    case 9: result = audit_nullable_value(status == 0 ? NULL : &value); break;
    default: return 91;
    }
    printf("CALL_RETURN %d\n", result);
    return 0;
}
/* end of c2fb-call-termination.c */
