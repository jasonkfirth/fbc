/*
 * Project: FreeBASIC external library checks
 * File: ffi-layout.c
 * Purpose: Report the ABI published by the native libffi development headers.
 * Responsibilities: Structure sizes, field offsets and target constants.
 * This file does not allocate closures or provide a libffi implementation.
 */
#include <ffi.h>
#include <stddef.h>
#include <stdio.h>

#define REPORT(name, value) printf(name "=%llu\n", (unsigned long long)(value))

int main(void)
{
    REPORT("pointer", sizeof(void *));
    REPORT("ffi_arg", sizeof(ffi_arg));
    REPORT("ffi_type", sizeof(ffi_type));
    REPORT("ffi_type.elements", offsetof(ffi_type, elements));
    REPORT("ffi_cif", sizeof(ffi_cif));
    REPORT("ffi_cif.arg_types", offsetof(ffi_cif, arg_types));
    REPORT("ffi_cif.rtype", offsetof(ffi_cif, rtype));
    REPORT("ffi_cif.bytes", offsetof(ffi_cif, bytes));
    REPORT("ffi_cif.flags", offsetof(ffi_cif, flags));
    REPORT("ffi_closure", sizeof(ffi_closure));
    REPORT("ffi_closure.cif", offsetof(ffi_closure, cif));
    REPORT("ffi_closure.fun", offsetof(ffi_closure, fun));
    REPORT("ffi_closure.user_data", offsetof(ffi_closure, user_data));
    REPORT("FFI_FIRST_ABI", FFI_FIRST_ABI);
    REPORT("FFI_LAST_ABI", FFI_LAST_ABI);
    REPORT("FFI_DEFAULT_ABI", FFI_DEFAULT_ABI);
    REPORT("FFI_TRAMPOLINE_SIZE", FFI_TRAMPOLINE_SIZE);
    return 0;
}

/* end of ffi-layout.c */
