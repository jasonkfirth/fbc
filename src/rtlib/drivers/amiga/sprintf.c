/*
    Project: FreeBASIC AmigaOS runtime
    ------------------------

    File: drivers/amiga/sprintf.c
    Purpose: Preserve the C variadic ABI when formatting strings.
    Responsibilities: Forward sprintf to newlib's standard C formatter.
    This file intentionally does NOT contain: formatting or DOS RawDoFmt.
*/

#include <stdarg.h>
#include <stdio.h>

/* libamiga supplies a legacy sprintf which calls Exec's RawDoFmt. Its %d
   consumes a 16-bit word, whereas this SDK's C int and promoted arguments
   occupy 32 bits. The runtime archive precedes libamiga, so this definition
   keeps C callers on newlib's ABI without changing native tag-list helpers. */
int sprintf(char *destination, const char *format, ...)
{
    va_list arguments;
    int result;

    va_start(arguments, format);
    result = vsprintf(destination, format, arguments);
    va_end(arguments);
    return result;
}

/* end of drivers/amiga/sprintf.c */
