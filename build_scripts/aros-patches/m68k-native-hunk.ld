/*
    Project: FreeBASIC AROS release workflow
    ----------------------------------------

    File: m68k-native-hunk.ld

    Purpose:

        Consolidate AROS m68k native development programs into the three
        loadable regions represented by the classic Amiga Hunk format.

    Responsibilities:

        - place code, constants, exception tables, and unwind data together
        - preserve writable data and zero-filled storage as separate regions
        - make PC-relative metadata references local to a single Hunk region

    This file intentionally does NOT define final runtime addresses or change
    the link layout used for programs produced by the native toolchain.
*/

SECTIONS
{
    .text :
    {
        *(.text)
        *(.text.*)
        *(.rodata)
        *(.rodata.*)
        *(.gcc_except_table*)
        *(.eh_frame)
        *(.eh_frame.*)
    }

    .data :
    {
        *(.data)
        *(.data.*)
    }

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(COMMON)
    }
}

/* end of m68k-native-hunk.ld */
