/*
    FreeBASIC native library regression checks
    File: quicklz-abi.c
    Purpose: Obtain state layout facts from the matching QuickLZ C headers.
    Responsibilities: Report fixed-width hashes, native state sizes and offsets.
    This file contains no replacement declarations or compression implementation.
*/

#include <stddef.h>
#include <stdio.h>
#include "quicklz.h"

int main(void)
{
    printf("ABI %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu %zu\n",
           sizeof(ui32), sizeof(ui16), sizeof(qlz_hash_compress),
           offsetof(qlz_hash_compress, cache), offsetof(qlz_hash_compress, offset),
           sizeof(qlz_hash_decompress), sizeof(qlz_state_compress),
           offsetof(qlz_state_compress, hash), offsetof(qlz_state_compress, hash_counter),
           sizeof(qlz_state_decompress), offsetof(qlz_state_decompress, hash_counter),
           offsetof(qlz_state_decompress, stream_counter));
    return 0;
}

/* end of quicklz-abi.c */
