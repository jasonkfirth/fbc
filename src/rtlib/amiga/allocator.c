/*
    FreeBASIC classic AmigaOS runtime
    --------------------------------

    File: amiga/allocator.c

    Purpose:
        Provide an Exec-backed allocator usable throughout SDK startup.

    Responsibilities:
        - check allocation sizes and preserve the size required by FreeMem
        - implement the public and reentrant newlib allocation interfaces
        - preserve failed reallocations and avoid overflowing calloc products

    This file intentionally does NOT contain:
        - a private free list, garbage collection, or C++ allocator state

    The pinned SDK initializes libpthread before its C++ heap constructor.
    pthread's initial allocation therefore reaches an uninitialized heap.
    Exec already serializes AllocMem/FreeMem, so this allocator needs no
    application lock and works before constructors and on worker tasks.

    Allocation layout:
        0..3  : total native allocation size
        4..7  : requested payload size
        8..15 : padding which preserves Exec's eight-byte alignment
        16..  : caller-owned payload
*/

#include "../fb.h"

#include <exec/memory.h>
#include <proto/exec.h>
#include <reent.h>

typedef struct AMIGA_ALLOCATION
{
    ULONG allocated;
    ULONG requested;
    ULONG padding[2];
} AMIGA_ALLOCATION;

/* Referenced by runtime startup so archive selection cannot discard the
   allocator in favour of the SDK's constructor-dependent implementation. */
void fb_hAmigaInitAllocator(void) {}

void *_malloc_r(struct _reent *context, size_t size)
{
    AMIGA_ALLOCATION *header;
    size_t payload = size > 0 ? size : 1;

    if (payload > SIZE_MAX - sizeof(*header)) {
        if (context != NULL) context->_errno = ENOMEM;
        return NULL;
    }
    header = AllocMem((ULONG)(payload + sizeof(*header)), MEMF_PUBLIC);
    if (header == NULL) {
        if (context != NULL) context->_errno = ENOMEM;
        return NULL;
    }
    header->allocated = (ULONG)(payload + sizeof(*header));
    header->requested = (ULONG)size;
    return header + 1;
}

void _free_r(struct _reent *context, void *pointer)
{
    AMIGA_ALLOCATION *header;

    (void)context;
    if (pointer == NULL) return;
    header = (AMIGA_ALLOCATION *)pointer - 1;
    FreeMem(header, header->allocated);
}

void *_calloc_r(struct _reent *context, size_t count, size_t size)
{
    void *pointer;
    size_t bytes;

    if (count != 0 && size > SIZE_MAX / count) {
        if (context != NULL) context->_errno = ENOMEM;
        return NULL;
    }
    bytes = count * size;
    pointer = _malloc_r(context, bytes);
    if (pointer != NULL && bytes != 0) memset(pointer, 0, bytes);
    return pointer;
}

void *_realloc_r(struct _reent *context, void *pointer, size_t size)
{
    AMIGA_ALLOCATION *header;
    void *replacement;
    size_t copied;

    if (pointer == NULL) return _malloc_r(context, size);
    if (size == 0) { _free_r(context, pointer); return NULL; }
    replacement = _malloc_r(context, size);
    if (replacement == NULL) return NULL;
    header = (AMIGA_ALLOCATION *)pointer - 1;
    copied = header->requested < size ? header->requested : size;
    if (copied != 0) memcpy(replacement, pointer, copied);
    _free_r(context, pointer);
    return replacement;
}

void *malloc(size_t size) { return _malloc_r(_REENT, size); }
void *calloc(size_t count, size_t size) { return _calloc_r(_REENT, count, size); }
void *realloc(void *pointer, size_t size) { return _realloc_r(_REENT, pointer, size); }
void free(void *pointer) { _free_r(_REENT, pointer); }

/* end of amiga/allocator.c */
