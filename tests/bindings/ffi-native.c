/* FreeBASIC libffi native qualification
 * File: ffi-native.c
 * Purpose: Compare actual closure layouts and execute narrow integer calls.
 * Responsibilities: Guarded result storage, native callbacks and owned cleanup.
 * This file contains no trampoline implementation or simulated libffi calls.
 */
#include <ffi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct closure_alignment { unsigned char tag; ffi_closure item; };
struct result_guard { uint64_t before; ffi_arg value; uint64_t after; };
static uint32_t scalar(const char *text)
{
    return text && !strcmp(text, "ffi") ? UINT32_MAX : 0;
}
static void callback(ffi_cif *cif, void *result, void **args, void *data)
{
    (void)cif;
    *(ffi_arg *)result = scalar(*(const char **)args[0]);
    (*(unsigned *)data)++;
}
int main(int argc, char **argv)
{
    ffi_type *types[1] = {&ffi_type_pointer};
    const char *text = "ffi";
    void *values[1] = {&text}, *code = NULL;
    ffi_cif cif;
    ffi_closure *closure;
    unsigned calls = 0;
    uint32_t (*function)(const char *);
    struct result_guard result = {UINT64_C(0x123456789abcdef0), 0, UINT64_C(0xfedcba9876543210)};
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %d %d\n",
           sizeof(ffi_arg), sizeof(ffi_type), sizeof(ffi_cif), sizeof(ffi_closure),
           offsetof(ffi_cif, arg_types), offsetof(ffi_cif, flags),
           offsetof(ffi_closure, cif), offsetof(ffi_closure, fun), offsetof(ffi_closure, user_data),
           offsetof(struct closure_alignment, item), FFI_TRAMPOLINE_SIZE, FFI_DEFAULT_ABI);
    /* The controller compares layouts before permitting closure execution. */
    if (argc == 2 && !strcmp(argv[1], "abi")) return 0;
    if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint32, types) != FFI_OK) return 1;
    ffi_call(&cif, FFI_FN(scalar), &result.value, values);
    if (result.value != UINT32_MAX || result.before != UINT64_C(0x123456789abcdef0) ||
        result.after != UINT64_C(0xfedcba9876543210)) return 2;
    closure = ffi_closure_alloc(sizeof(*closure), &code);
    if (!closure) return 3;
    if (ffi_prep_closure_loc(closure, &cif, callback, &calls, code) != FFI_OK) {
        ffi_closure_free(closure);
        return 4;
    }
    function = (uint32_t (*)(const char *))code;
    if (function("ffi") != UINT32_MAX || calls != 1) {
        ffi_closure_free(closure);
        return 5;
    }
    ffi_closure_free(closure);
    puts("LIBFFI_NATIVE=PASS uint32=4294967295 guards=2 closure=1 cleanup=1");
    return 0;
}
/* end of ffi-native.c */
