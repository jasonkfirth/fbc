/*
 * Project: FreeBASIC runtime library
 * File: system/sys_exec.c
 * Purpose: Provide sys exec runtime support.
 * Responsibilities: Process, environment, path and operating-system services.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

FBCALL int fb_Exec( FBSTRING *program, FBSTRING *args )
{
    return fb_ExecEx( program, args, TRUE );
}

/* end of system/sys_exec.c */
