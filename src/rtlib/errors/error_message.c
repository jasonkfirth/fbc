/*
 * Project: FreeBASIC runtime library
 * File: errors/error_message.c
 * Purpose: error message buffer shared by fb_Die() and fb_Assert() functions.
 * Responsibilities: Runtime errors, assertions and fault reporting.
 * This file intentionally does NOT contain: Compiler parsing or unrelated runtime subsystems.
 */
#include "fb.h"

char __fb_errmsg[FB_ERRMSG_SIZE] = "";

/* end of errors/error_message.c */
