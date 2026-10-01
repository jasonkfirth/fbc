/*
    FreeBASIC AmigaOS test transport
    --------------------------------

    File: guest-entry.s
    Purpose: Put a callable entry at the first byte of the Hunk code segment.
    Responsibilities: Transfer the DOS entry to the C guest test controller.
    This file intentionally does NOT contain C runtime initialization.

    Hunk executables start at the first code segment, without an ELF entry
    address. Link this object before C objects, whose strings may precede code.
*/

.text
.globl _start
_start:
    jmp _fb_amigaGuestMain

/* end of guest-entry.s */
