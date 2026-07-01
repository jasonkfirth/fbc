/*
    FreeBASIC runtime library
    -------------------------

    File: fb_nuttx_file_array.c

    Purpose:

        Provide array PUT support for the small NuttX runtime.

    Responsibilities:

        - calculate the contiguous byte count represented by a BASIC array
        - forward the actual write to the existing file PUT implementation

    This file intentionally does NOT contain:

        - array allocation
        - descriptor construction
        - file handle management
*/

#include "fb.h"

#include <stdint.h>

/* ------------------------------------------------------------------------- */
/* Array byte accounting                                                     */
/* ------------------------------------------------------------------------- */

static int32 fb_nuttx_array_total_bytes(const FB_NUTTX_ARRAY *array,
    uint32 *bytes)
{
    int32 i;
    uint64_t elements;
    uint64_t total;

    if (bytes == NULL)
        return -1;

    *bytes = 0;

    if ((array == NULL) || (array->data == NULL))
        return -1;

    if (array->element_len <= 0)
        return -1;

    elements = 1;

    for (i = 0; i < array->dimensions; i++) {
        if (array->dimtb[i].elements <= 0)
            return 0;

        elements *= (uint64_t)array->dimtb[i].elements;

        if (elements > UINT32_MAX)
            return -1;
    }

    total = elements * (uint64_t)array->element_len;

    if (total > UINT32_MAX)
        return -1;

    *bytes = (uint32)total;
    return 0;
}

/* ------------------------------------------------------------------------- */
/* PUT #, , array()                                                          */
/* ------------------------------------------------------------------------- */

int32 fb_FilePutArray(const int32 file_num, const int32 position,
    const FB_NUTTX_ARRAY *array)
{
    uint32 bytes;

    if (fb_nuttx_array_total_bytes(array, &bytes) != 0)
        return -1;

    if (bytes == 0)
        return 0;

    return fb_FilePut(file_num, position, array->data, bytes);
}

/* end of fb_nuttx_file_array.c */
