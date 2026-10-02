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
        8..11 : previous live allocation
        12..15: next live allocation
        16..  : caller-owned payload
*/

#include "../fb.h"

#include <exec/memory.h>
#include <proto/exec.h>
#include <reent.h>
#include <stabs.h>

typedef struct AMIGA_ALLOCATION
{
    ULONG allocated;
    ULONG requested;
    struct AMIGA_ALLOCATION *previous, *next;
} AMIGA_ALLOCATION;

/* Exec does not reclaim arbitrary AllocMem blocks when a command returns.
   Track this command's live allocations so unclosed C-library contexts and
   application allocations cannot survive UnLoadSeg. Workers are joined before
   the late cleanup hook. Forbid protects only bounded list updates, with no
   allocator or DOS call inside that task-switch exclusion. */
static AMIGA_ALLOCATION *allocations;

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
    Forbid();
    header->previous = NULL;
    header->next = allocations;
    if (allocations != NULL) allocations->previous = header;
    allocations = header;
    Permit();
    return header + 1;
}

void _free_r(struct _reent *context, void *pointer)
{
    AMIGA_ALLOCATION *header;

    (void)context;
    if (pointer == NULL) return;
    header = (AMIGA_ALLOCATION *)pointer - 1;
    Forbid();
    if (header->previous != NULL) header->previous->next = header->next;
    else allocations = header->next;
    if (header->next != NULL) header->next->previous = header->previous;
    Permit();
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

__attribute__((used)) static void release_allocations(void)
{
    AMIGA_ALLOCATION *header;
    Forbid();
    header = allocations;
    allocations = NULL;
    Permit();
    while (header != NULL) {
        AMIGA_ALLOCATION *next = header->next;
        FreeMem(header, header->allocated);
        header = next;
    }
}

/* All C/BASIC destructors, pthread joins, and descriptor cleanup precede this.
   The SDK closes its library bases at -100, after these final Exec frees. */
ADD2EXIT(release_allocations, -99);

/* end of amiga/allocator.c */
