/*
 * Project: FreeBASIC runtime library
 * File: drivers/unix/io_scroll.c
 * Purpose: console scrolling for when VIEW is used.
 * Responsibilities: Target-specific runtime support for unix.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include "fb_private_console.h"

void fb_ConsoleScroll(int nrows)
{
	fb_hTermOut(SEQ_SCROLL, 0, nrows);
}

/* end of drivers/unix/io_scroll.c */
