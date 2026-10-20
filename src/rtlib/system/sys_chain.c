/*
 * Project: FreeBASIC runtime library
 * File: system/sys_chain.c
 * Purpose: chain function.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_Chain ( FBSTRING *program )
{
    return fb_ExecEx( program, NULL, TRUE );
}

/* end of system/sys_chain.c */
