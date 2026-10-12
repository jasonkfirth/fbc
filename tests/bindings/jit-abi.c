/*
    FreeBASIC native library regression checks
    File: jit-abi.c
    Purpose: Obtain ABI facts from the matching GNU libjit headers.
    Responsibilities: Scalar widths, constant layout and memory manager callbacks.
    This file contains no replacement declarations or JIT implementation.
*/

#include <stddef.h>
#include <stdio.h>
#include <jit/jit.h>

struct constant_alignment {
    char prefix;
    jit_constant_t value;
};

/* Values above 32 bits detect truncated callback parameters without allocating
   a large buffer. The FreeBASIC callback returns a caller-owned marker. */
int fbc_jit_check_callback(jit_memory_manager_t manager, void *marker)
{
    jit_size_t size = (jit_size_t)33;
    jit_size_t align = (jit_size_t)16;
    if (sizeof(jit_size_t) > 4) {
        size += (jit_size_t)0x100000000ULL;
        align += (jit_size_t)0x100000000ULL;
    }
    if (!manager || !manager->alloc_data)
        return 0;
    return manager->alloc_data(marker, size, align) == marker;
}

#ifndef JIT_ABI_HELPER
int main(void)
{
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(jit_int), sizeof(jit_uint), sizeof(jit_nint),
           sizeof(jit_nuint), sizeof(jit_size_t), sizeof(jit_label_t),
           sizeof(jit_nfloat), sizeof(jit_constant_t),
           offsetof(jit_constant_t, un), offsetof(struct constant_alignment, value),
           sizeof(struct jit_memory_manager), offsetof(struct jit_memory_manager, alloc_data));
    /* Installed headers alone cannot prove that a DLL was built with them. */
    if (jit_type_get_size(jit_type_int) != sizeof(jit_int) ||
        jit_type_get_size(jit_type_nint) != sizeof(jit_nint) ||
        jit_type_get_size(jit_type_long) != sizeof(jit_long) ||
        jit_type_get_size(jit_type_nfloat) != sizeof(jit_nfloat)) {
        puts("LIBJIT_C_TYPES=MISMATCH");
        return 1;
    }
    puts("LIBJIT_C_TYPES=PASS");
    return 0;
}
#endif

/* end of jit-abi.c */
