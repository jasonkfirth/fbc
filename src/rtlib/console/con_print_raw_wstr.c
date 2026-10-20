/*
 * Project: FreeBASIC runtime library
 * File: console/con_print_raw_wstr.c
 * Purpose: print raw data - no interpretation is done.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

#define FB_CONPRINTRAW fb_ConPrintRawWstr
#define FB_TCHAR FB_WCHAR
#define FB_CON_HOOK_TWRITE Write
#define FB_TCHAR_ADVANCE( iter, count ) \
    iter += count

#include "console/con_print_raw_uni.h"

/* end of console/con_print_raw_wstr.c */
