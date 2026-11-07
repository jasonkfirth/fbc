/*
    Project: FreeBASIC compiler regression tests
    File: reserved-abi-helper.c
    Purpose: Provide external symbols for the C identifier fixture.
    Responsibilities: Define a function and a variable with fixed object names.
    This fixture does not depend on the FreeBASIC runtime.
*/

#if defined(_WIN32) && !defined(_WIN64)
#define ADD_SYMBOL "__fb_test_external_add"
#define DATA_SYMBOL "___fb_test_external_value"
#else
#define ADD_SYMBOL "_fb_test_external_add"
#define DATA_SYMBOL "__fb_test_external_value"
#endif

int fb_test_inspect_field(const void *record);

#if !defined(FB_TEST_EXTERNAL_PROVIDER)
int fixture_add(int value) __asm__(ADD_SYMBOL);
int fixture_data __asm__(DATA_SYMBOL) = 17;

int fixture_add(int value)
{
    return value + 5;
}
#endif

#if defined(FB_TEST_EMULATE_BUILTIN)
#if defined(_WIN32) && !defined(_WIN64)
#define BUILTIN_SYMBOL "___builtin_popcount"
#else
#define BUILTIN_SYMBOL "__builtin_popcount"
#endif

/* The native backend needs a callable implementation of this C builtin. */
int fixture_popcount(unsigned int value) __asm__(BUILTIN_SYMBOL);

int fixture_popcount(unsigned int value)
{
    int count = 0;
    while (value != 0) {
        count += (int)(value & 1u);
        value >>= 1;
    }
    return count;
}
#endif

int fb_test_inspect_field(const void *record)
{
    int value;
    __builtin_memcpy(&value, record, sizeof(value));
    return value;
}

/* end of reserved-abi-helper.c */
