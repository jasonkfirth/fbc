/*
 * Project: FreeBASIC C backend regression tests
 * File: c-code-pointer-load.c
 * Purpose: Observe typed native callbacks carried through an opaque foreign API.
 * Responsibilities: Observe lookups, restored BASIC callbacks and actual calls.
 * This file intentionally does NOT use object/function pointer casts.
 */
typedef __INT32_TYPE__ fb_test_value;
typedef fb_test_value (*fb_test_callback)(fb_test_value);
typedef fb_test_value (*fb_test_ref_callback)(fb_test_value *);
#if defined(__i386__)
typedef fb_test_value (__attribute__((stdcall)) *fb_test_std_callback)(fb_test_value);
#else
typedef fb_test_callback fb_test_std_callback;
#endif

void *fb_test_code_address(fb_test_value which);
fb_test_value fb_test_lookup_count(fb_test_value which);
fb_test_value fb_test_call_count(void);
fb_test_value fb_test_accept_address(void *address, fb_test_value value);
fb_test_value fb_test_accept_ref_address(void *address, fb_test_value *value);
fb_test_value fb_test_accept_std_address(void *address, fb_test_value value);

static fb_test_value lookups[3];
static fb_test_value calls;

static fb_test_value add_one(fb_test_value value)
{
    calls++;
    return value + 1;
}

static fb_test_value add_hundred(fb_test_value value)
{
    calls++;
    return value + 100;
}

void *fb_test_code_address(fb_test_value which)
{
    fb_test_callback callback;
    void *address;
    _Static_assert(sizeof(callback) == sizeof(address), "native code-address ABI");

    if (which < 0 || which >= 3)
        return 0;
    lookups[which]++;
    if (which == 0)
        return 0;
    callback = which == 2 ? add_hundred : add_one;
    __builtin_memcpy(&address, &callback, sizeof(address));
    return address;
}

fb_test_value fb_test_lookup_count(fb_test_value which)
{
    if (which < 0 || which >= 3)
        return -1;
    return lookups[which];
}

fb_test_value fb_test_call_count(void)
{
    return calls;
}

fb_test_value fb_test_accept_address(void *address, fb_test_value value)
{
    fb_test_callback callback;
    _Static_assert(sizeof(callback) == sizeof(address), "native code-address ABI");

    if (address == 0)
        return -1;
    __builtin_memcpy(&callback, &address, sizeof(callback));
    return callback(value);
}

fb_test_value fb_test_accept_ref_address(void *address, fb_test_value *value)
{
    fb_test_ref_callback callback;
    _Static_assert(sizeof(callback) == sizeof(address), "native code-address ABI");

    if (address == 0 || value == 0)
        return -1;
    __builtin_memcpy(&callback, &address, sizeof(callback));
    return callback(value);
}

fb_test_value fb_test_accept_std_address(void *address, fb_test_value value)
{
    fb_test_std_callback callback;
    _Static_assert(sizeof(callback) == sizeof(address), "native code-address ABI");

    if (address == 0)
        return -1;
    __builtin_memcpy(&callback, &address, sizeof(callback));
    return callback(value);
}

/* end of c-code-pointer-load.c */
