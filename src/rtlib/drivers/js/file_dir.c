/*
 * Project: FreeBASIC runtime library
 * File: drivers/js/file_dir.c
 * Purpose: Enumerate files in the Emscripten virtual filesystem.
 * Responsibilities: Select the shared POSIX directory implementation.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
/* Emscripten provides opendir(), readdir() and stat() over the virtual
 * filesystem, so the Unix implementation also works for embedded assets. */
#include "../unix/file_dir.c"

/* end of drivers/js/file_dir.c */
