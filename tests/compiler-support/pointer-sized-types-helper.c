/*
    Project: FreeBASIC C backend regression tests
    File: pointer-sized-types-helper.c

    Purpose: Check FreeBASIC calls against native pointer-sized C types.
    Responsibilities: Validate values and return types at the C ABI boundary.
    This helper does not test targets where it cannot be executed.
*/

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
/* Windows has no standard ssize_t, and FreeBASIC uses a pointer-sized signed type. */
typedef intptr_t fbc_ssize_type;
#else
#include <sys/types.h>
typedef ssize_t fbc_ssize_type;
#endif

/* Distinct flags show that each C entry point received the expected value. */
enum {
    SIZE_SEEN = 1u << 0,
    SSIZE_SEEN = 1u << 1,
    DIFFERENCE_SEEN = 1u << 2,
    INTPTR_SEEN = 1u << 3,
    UINTPTR_SEEN = 1u << 4,
    ALL_SEEN = SIZE_SEEN | SSIZE_SEEN | DIFFERENCE_SEEN | INTPTR_SEEN | UINTPTR_SEEN
};

size_t fbc_test_size(size_t value);
fbc_ssize_type fbc_test_ssize(fbc_ssize_type value);
ptrdiff_t fbc_test_difference(ptrdiff_t value);
intptr_t fbc_test_intptr(intptr_t value);
uintptr_t fbc_test_uintptr(uintptr_t value);
extern void fbc_check_pointer_types(void);

static unsigned int fbc_seen;

size_t fbc_test_size(size_t value)
{
    if (value == SIZE_MAX)
        fbc_seen |= SIZE_SEEN;
    return value;
}

fbc_ssize_type fbc_test_ssize(fbc_ssize_type value)
{
    if (value == -2)
        fbc_seen |= SSIZE_SEEN;
    return value;
}

ptrdiff_t fbc_test_difference(ptrdiff_t value)
{
    if (value == -3)
        fbc_seen |= DIFFERENCE_SEEN;
    return value;
}

intptr_t fbc_test_intptr(intptr_t value)
{
    if (value == -4)
        fbc_seen |= INTPTR_SEEN;
    return value;
}

uintptr_t fbc_test_uintptr(uintptr_t value)
{
    if (value == UINTPTR_MAX)
        fbc_seen |= UINTPTR_SEEN;
    return value;
}

int main(void)
{
    fbc_check_pointer_types();
    return fbc_seen == ALL_SEEN ? 0 : 1;
}

/* end of pointer-sized-types-helper.c */
