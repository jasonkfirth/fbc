/*
    Project: FreeBASIC Unix runtime ABI regression
    File: vt100-symbol-legacy.c
    Purpose: Supply the preceding declaration and an application flag override.
    Responsibilities: Retain the original symbol, int type and mutable identity.
    The current header belongs to the separate native client translation unit.
*/
extern int __fb_enable_vt100_escapes;

#ifdef VT100_TEST_OVERRIDE
int __fb_enable_vt100_escapes = 43;
#endif

const void *fb_test_vt100_legacy_address(void);
int fb_test_vt100_legacy_read(void);
void fb_test_vt100_legacy_write(int value);

const void *fb_test_vt100_legacy_address(void)
{
    return &__fb_enable_vt100_escapes;
}

int fb_test_vt100_legacy_read(void)
{
    return __fb_enable_vt100_escapes;
}

void fb_test_vt100_legacy_write(int value)
{
    __fb_enable_vt100_escapes = value;
}

/* end of vt100-symbol-legacy.c */
