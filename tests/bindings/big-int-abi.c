/*
    FreeBASIC native library regression checks
    File: big-int-abi.c
    Purpose: Obtain layout facts from the matching big_int C headers.
    Responsibilities: Report limb, enum, size_t and structure widths.
    This file contains no replacement declarations or arithmetic implementation.
*/

#include <stddef.h>
#include <stdio.h>
#include "big_int.h"
#include "str_types.h"

int main(void)
{
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(big_int_word), sizeof(big_int_dword), sizeof(sign_type),
           sizeof(bin_op_type), sizeof(size_t), sizeof(big_int),
           offsetof(big_int, sign), offsetof(big_int, len),
           offsetof(big_int, len_allocated), sizeof(big_int_str),
           offsetof(big_int_str, len), offsetof(big_int_str, len_allocated));
    return 0;
}

/* end of big-int-abi.c */
