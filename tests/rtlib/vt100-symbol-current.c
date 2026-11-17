/*
    Project: FreeBASIC Unix runtime ABI regression
    File: vt100-symbol-current.c
    Purpose: Check the current C name against the existing VT100 flag symbol.
    Responsibilities: Compare addresses, defaults, overrides and live updates.
    Terminal output and device initialization retain their runtime tests.
*/
#include "fb.h"
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif
const void *fb_test_vt100_legacy_address(void);
int fb_test_vt100_legacy_read(void);
void fb_test_vt100_legacy_write(int value);
#ifdef __cplusplus
}
#endif

#define CHECK(condition) do { if (!(condition)) return __LINE__; } while (0)

int main(void)
{
    static const int values[] = {-1, 0, 1, 2, INT_MIN, INT_MAX, 43};
    CHECK(fb_test_vt100_legacy_address() == &fb_enable_vt100_escapes);
#ifdef VT100_TEST_OVERRIDE
    CHECK(fb_enable_vt100_escapes == 43);
#else
    CHECK(fb_enable_vt100_escapes == -1);
#endif
    for (size_t index = 0; index < sizeof(values) / sizeof(values[0]); index++) {
        fb_enable_vt100_escapes = values[index];
        CHECK(fb_test_vt100_legacy_read() == values[index]);
        fb_test_vt100_legacy_write(values[(index + 1) % (sizeof(values) / sizeof(values[0]))]);
        CHECK(fb_enable_vt100_escapes == values[(index + 1) % (sizeof(values) / sizeof(values[0]))]);
        CHECK((fb_enable_vt100_escapes != 0) == (fb_test_vt100_legacy_read() != 0));
    }
    puts("VT100_SYMBOL_PASS");
    return 0;
}

/* end of vt100-symbol-current.c */
