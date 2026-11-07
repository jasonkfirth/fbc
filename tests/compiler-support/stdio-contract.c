/*
 * Project: FreeBASIC C backend regression tests
 * File: stdio-contract.c
 * Purpose: Check a BASIC stream through the platform's native stdio header.
 * Responsibilities: Read the FILE-named export and append through native FILE*.
 * This file intentionally does NOT define a surrogate stream or mock libc.
 */

#include <stdio.h>

#if (defined(_WIN32) && !defined(_WIN64)) || defined(__APPLE__)
#define FILE_EXPORT_SYMBOL "_FILE"
#else
#define FILE_EXPORT_SYMBOL "FILE"
#endif

#ifdef STDIO_FILE_PROC
extern int fixture_file_export(void) __asm__(FILE_EXPORT_SYMBOL);
#else
extern int fixture_file_export __asm__(FILE_EXPORT_SYMBOL);
#endif

int fixture_stdio_collision(void);
int fixture_stdio_suffix(FILE *stream);

int fixture_stdio_collision(void)
{
#ifdef STDIO_FILE_PROC
    return fixture_file_export();
#else
    return fixture_file_export;
#endif
}

int fixture_stdio_suffix(FILE *stream)
{
    return fputc('!', stream) == EOF ? -1 : 0;
}

/* end of stdio-contract.c */
