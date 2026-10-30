/*
    Project: FreeBASIC C backend regression controls
    File: c-private-global-storage-caller.c
    Purpose: Read BASIC public storage through its native object identity.
    Responsibilities: Check public data that the BASIC body does not read.
    This file intentionally does NOT inspect private globals or invoke callbacks.
*/
typedef __INT32_TYPE__ fb_test_value;
extern fb_test_value fb_test_public_storage;
fb_test_value fb_test_public_value(void);

fb_test_value fb_test_public_value(void)
{
    return fb_test_public_storage;
}

/* end of c-private-global-storage-caller.c */
