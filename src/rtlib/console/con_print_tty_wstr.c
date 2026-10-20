/*
 * Project: FreeBASIC runtime library
 * File: console/con_print_tty_wstr.c
 * Purpose: print wide text data - using TTY (teletype) interpretation.
 * Responsibilities: Console input, output, cursor state and PRINT formatting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

#define FB_CONPRINTTTY                  fb_ConPrintTTYWstr
#define FB_CONPRINTRAW                  fb_ConPrintRawWstr
#define FB_TCHAR                        FB_WCHAR
#define FB_TCHAR_GET(p)                 (*(p))
#define FB_TCHAR_ADVANCE(i,c)           i += c
#define FB_TCHAR_GET_CHAR_SIZE(p)       1
#define _TC(c)                          _LC(c)

#include "console/con_print_tty_uni.h"

/* end of console/con_print_tty_wstr.c */
