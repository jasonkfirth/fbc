/'
    Project: FreeBASIC Coreutils library regression
    File: coreutils-obstack.bas
    Purpose: Expose the translated library's canonical layout and callback slot.
    Responsibilities: Include the complete production module for native callers.
    The C observer owns allocations, callback behavior and process checks.
'/
#lang "fb"
#include once "obstack_553085db50f5_candidates.bas"

Function probeSetHandler CDecl Alias "fb_test_obstack_set_handler" _
    (ByVal callback As c2fb_callback_16) As c2fb_callback_16
    Dim previous As c2fb_callback_16 = c2fb_global_obstack_alloc_failed_handler
    c2fb_global_obstack_alloc_failed_handler = callback
    Return previous
End Function

Sub probeLayout CDecl Alias "fb_test_obstack_layout" (ByVal values As ULongInt Ptr)
    Dim value As c2fb_obstack
    Dim address As ULongInt = CULngInt(@value)
    values[0] = SizeOf(value)
    values[1] = CULngInt(@value.chunk) - address
    values[2] = CULngInt(@value.object_base) - address
    values[3] = CULngInt(@value.next_free) - address
    values[4] = CULngInt(@value.chunk_limit) - address
    values[5] = CULngInt(@value.alignment_mask) - address
    values[6] = CULngInt(@value.chunkfun) - address
    values[7] = CULngInt(@value.freefun) - address
    values[8] = CULngInt(@value.extra_arg) - address
End Sub

Function probeFlags CDecl Alias "fb_test_obstack_flags" _
    (ByVal value As c2fb_obstack Ptr) As ULong
    Return value->use_extra_arg Or (value->maybe_empty_object Shl 1) Or _
        (value->alloc_failed Shl 2)
End Function

/' end of coreutils-obstack.bas '/
