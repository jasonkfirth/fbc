/*
    Project: FreeBASIC runtime library
    ----------------------------------

    File: fb_nuttx_array.c

    Purpose:

        Provide one section of the small NuttX runtime used by the
        generated-C RISC-V smoke target.

    This file is included by fb_nuttx_minrt.c while the target is still
    being brought up. Keeping the command bodies in smaller files makes
    the code easier to compare with the normal rtlib layout.
*/

int32 fb_ArrayErase(FB_NUTTX_ARRAY *array)
{
    if (array == NULL)
        return -1;

    if (array->ptr != NULL)
        free(array->ptr);

    array->data = NULL;
    array->ptr = NULL;
    array->size = 0;

    return 0;
}

int32 fb_ArrayClear(FB_NUTTX_ARRAY *array)
{
    if (array == NULL)
        return -1;

    if ((array->ptr != NULL) && (array->size > 0))
        memset(array->ptr, 0, (size_t)array->size);

    return 0;
}

size_t fb_ArrayLen(FB_NUTTX_ARRAY *array)
{
    if ((array == NULL) || (array->ptr == NULL) || (array->element_len <= 0))
        return 0;

    return (size_t)array->size / (size_t)array->element_len;
}

size_t fb_ArraySize(FB_NUTTX_ARRAY *array)
{
    if ((array == NULL) || (array->ptr == NULL) || (array->size <= 0))
        return 0;

    return (size_t)array->size;
}

typedef void (*FB_NUTTX_ARRAY_PROC)(void *);

static void fb_nuttx_array_call_proc(void *base, const size_t element_len,
    const size_t first, const size_t count, void *proc)
{
    FB_NUTTX_ARRAY_PROC typed_proc;
    size_t i;

    if ((base == NULL) || (element_len == 0) || (proc == NULL))
        return;

    typed_proc = (FB_NUTTX_ARRAY_PROC)proc;

    for (i = 0; i < count; i++)
        typed_proc((char *)base + ((first + i) * element_len));
}

static size_t fb_nuttx_array_element_count(const FB_NUTTX_ARRAY *array)
{
    if ((array == NULL) || (array->element_len <= 0) || (array->size <= 0))
        return 0;

    return (size_t)array->size / (size_t)array->element_len;
}

static void fb_nuttx_array_destruct(FB_NUTTX_ARRAY *array, void *dtor)
{
    fb_nuttx_array_call_proc(array != NULL ? array->ptr : NULL,
        array != NULL ? (size_t)array->element_len : 0, 0,
        fb_nuttx_array_element_count(array), dtor);
}

static int32 fb_nuttx_array_redim_va(FB_NUTTX_ARRAY *array,
    const uint32 element_len, const uint32 dimensions, va_list args,
    void **old_ptr, size_t *old_bytes, size_t *old_elements)
{
    int32 lbound[8];
    int32 ubound[8];
    int32 elements[8];
    uint32 i;
    size_t bytes;
    size_t total_elements;
    char *allocation;
    intptr_t data_address;
    int64_t bias_elements;
    int64_t bias_bytes;
    int64_t stride;

    if ((array == NULL) || (element_len == 0) || (dimensions == 0) ||
        (dimensions > 8))
        return -1;

    total_elements = 1;

    for (i = 0; i < dimensions; i++) {
        int64_t count;

        lbound[i] = va_arg(args, int32);
        ubound[i] = va_arg(args, int32);

        if (ubound[i] < lbound[i]) {
            return -1;
        }

        count = (int64_t)ubound[i] - (int64_t)lbound[i] + 1;

        if ((count <= 0) || (count > INT32_MAX)) {
            return -1;
        }

        elements[i] = (int32)count;

        if (total_elements > (SIZE_MAX / (size_t)elements[i])) {
            return -1;
        }

        total_elements *= (size_t)elements[i];
    }

    bytes = total_elements * (size_t)element_len;

    if ((total_elements == 0) || ((bytes / (size_t)element_len) != total_elements))
        return -1;

    allocation = (char *)calloc(1, bytes);

    if (allocation == NULL)
        return -1;

    bias_elements = 0;
    stride = 1;

    for (i = dimensions; i > 0; i--) {
        bias_elements += (int64_t)lbound[i - 1] * stride;
        stride *= (int64_t)elements[i - 1];
    }

    bias_bytes = bias_elements * (int64_t)element_len;

    if ((bias_bytes > INTPTR_MAX) || (bias_bytes < INTPTR_MIN)) {
        free(allocation);
        return -1;
    }

    if (old_ptr != NULL) {
        *old_ptr = array->ptr;
        *old_bytes = (array->size > 0) ? (size_t)array->size : 0;
        *old_elements = fb_nuttx_array_element_count(array);
    } else {
        fb_ArrayErase(array);
    }

    array->ptr = allocation;
    data_address = (intptr_t)allocation - (intptr_t)bias_bytes;
    array->data = (void *)data_address;
    array->size = (int32)bytes;
    array->element_len = (int32)element_len;
    array->dimensions = (int32)dimensions;

    for (i = 0; i < dimensions; i++) {
        array->dimtb[i].elements = elements[i];
        array->dimtb[i].lbound = lbound[i];
        array->dimtb[i].ubound = ubound[i];
    }

    return 0;
}

int32 fb_ArrayRedimEx(FB_NUTTX_ARRAY *array, const uint32 element_len,
    const int32 doclear, const int32 isvarlen, const uint32 dimensions, ...)
{
    va_list args;
    int32 result;

    (void)doclear;
    (void)isvarlen;

    va_start(args, dimensions);
    result = fb_nuttx_array_redim_va(array, element_len, dimensions, args,
        NULL, NULL, NULL);
    va_end(args);

    return result;
}

int32 fb_ArrayEraseObj(FB_NUTTX_ARRAY *array, void *ctor, void *dtor)
{
    (void)ctor;

    fb_nuttx_array_destruct(array, dtor);

    return fb_ArrayErase(array);
}

int32 fb_ArrayRedimObj(FB_NUTTX_ARRAY *array, const uint32 element_len,
    void *ctor, void *dtor, const uint32 dimensions, ...)
{
    va_list args;
    int32 result;

    if (dtor != NULL)
        fb_nuttx_array_destruct(array, dtor);

    fb_ArrayErase(array);

    va_start(args, dimensions);
    result = fb_nuttx_array_redim_va(array, element_len, dimensions, args,
        NULL, NULL, NULL);
    va_end(args);

    if (result == 0)
        fb_nuttx_array_call_proc(array->ptr, element_len, 0,
            fb_nuttx_array_element_count(array), ctor);

    return result;
}

static int32 fb_nuttx_array_redim_preserve_va(FB_NUTTX_ARRAY *array,
    const uint32 element_len, const int32 isvarlen, void *ctor, void *dtor,
    const uint32 dimensions, va_list args)
{
    void *old_ptr;
    size_t old_bytes;
    size_t old_elements;
    size_t new_elements;
    size_t copy_bytes;
    void *remove_dtor;
    int32 result;

    result = fb_nuttx_array_redim_va(array, element_len, dimensions, args,
        &old_ptr, &old_bytes, &old_elements);

    if (result != 0)
        return result;

    new_elements = fb_nuttx_array_element_count(array);
    copy_bytes = old_bytes;

    if (copy_bytes > (size_t)array->size)
        copy_bytes = (size_t)array->size;

    if ((old_ptr != NULL) && (copy_bytes > 0))
        memcpy(array->ptr, old_ptr, copy_bytes);

    remove_dtor = dtor;

    if ((remove_dtor == NULL) && (isvarlen != 0))
        remove_dtor = (void *)fb_StrDelete;

    if (old_elements > new_elements)
        fb_nuttx_array_call_proc(old_ptr, element_len, new_elements,
            old_elements - new_elements, remove_dtor);
    else if (new_elements > old_elements)
        fb_nuttx_array_call_proc(array->ptr, element_len, old_elements,
            new_elements - old_elements, ctor);

    free(old_ptr);

    return 0;
}

int32 fb_ArrayRedimPresvEx(FB_NUTTX_ARRAY *array, const uint32 element_len,
    const int32 doclear, const int32 isvarlen, const uint32 dimensions, ...)
{
    va_list args;
    int32 result;

    (void)doclear;

    va_start(args, dimensions);
    result = fb_nuttx_array_redim_preserve_va(array, element_len, isvarlen,
        NULL, NULL, dimensions, args);
    va_end(args);

    return result;
}

int32 fb_ArrayRedimPresvObj(FB_NUTTX_ARRAY *array, const uint32 element_len,
    void *ctor, void *dtor, const uint32 dimensions, ...)
{
    va_list args;
    int32 result;

    va_start(args, dimensions);
    result = fb_nuttx_array_redim_preserve_va(array, element_len, 0, ctor,
        dtor, dimensions, args);
    va_end(args);

    return result;
}

int32 fb_ArrayLBound(FB_NUTTX_ARRAY *array, const int32 dimension)
{
    if ((array == NULL) || (dimension < 1) || (dimension > array->dimensions))
        return 0;

    return array->dimtb[dimension - 1].lbound;
}

int32 fb_ArrayUBound(FB_NUTTX_ARRAY *array, const int32 dimension)
{
    if ((array == NULL) || (dimension < 1) || (dimension > array->dimensions))
        return -1;

    return array->dimtb[dimension - 1].ubound;
}

/* ------------------------------------------------------------------------- */
/* Array destructor support                                                  */
/* ------------------------------------------------------------------------- */

void fb_hArrayDtorStr(FB_NUTTX_ARRAY *array, void *dtor, size_t keep_idx)
{
    FBSTRING *element;
    size_t elements;

    (void)dtor;

    if ((array == NULL) || (array->ptr == NULL))
        return;

    elements = fb_nuttx_array_element_count(array);

    if (keep_idx >= elements)
        return;

    /*
        String array cleanup must run from the end toward the beginning.
        This matches the shared rtlib behaviour and avoids surprises for
        future object-like string storage.
    */
    element = ((FBSTRING *)array->ptr) + (elements - 1);

    while (elements > keep_idx) {
        if (element->data != NULL)
            fb_StrDelete(element);

        element--;
        elements--;
    }
}

void fb_hArrayDtorObj(FB_NUTTX_ARRAY *array, void *dtor, size_t keep_idx)
{
    FB_NUTTX_ARRAY_PROC typed_dtor;
    unsigned char *element;
    size_t elements;
    size_t element_len;

    if ((array == NULL) || (array->ptr == NULL) || (dtor == NULL))
        return;

    typed_dtor = (FB_NUTTX_ARRAY_PROC)dtor;
    elements = fb_nuttx_array_element_count(array);

    if (keep_idx >= elements)
        return;

    element_len = array->element_len;

    if (element_len == 0)
        return;

    element = ((unsigned char *)array->ptr) + ((elements - 1) * element_len);

    while (elements > keep_idx) {
        typed_dtor(element);
        element -= element_len;
        elements--;
    }
}

void fb_ArrayDestructStr(FB_NUTTX_ARRAY *array)
{
    fb_hArrayDtorStr(array, NULL, 0);
}

void fb_ArrayStrErase(FB_NUTTX_ARRAY *array)
{
    fb_ArrayDestructStr(array);

    /*
        Fixed-length arrays own their storage outside the dynamic descriptor.
        Free only the normal dynamic array allocation, matching the hosted
        rtlib's FBARRAY_FLAGS_FIXED_LEN guard.
    */
    if ((array != NULL) && ((array->flags & 0x20) == 0))
        fb_ArrayErase(array);
}

void fb_ArrayDestructObj(FB_NUTTX_ARRAY *array, void *dtor)
{
    fb_hArrayDtorObj(array, dtor, 0);
}

/* end of fb_nuttx_array.c */
