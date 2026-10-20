/*
 * Project: FreeBASIC runtime library
 * File: drivers/dos/farmemset.c
 * Purpose: farptr memset routines for DOS.
 * Responsibilities: Target-specific runtime support for dos.
 * This file intentionally does NOT contain: Compiler parsing or portable runtime policy.
 */
#include "../../fb.h"
#include <sys/farptr.h>

void fb_hFarMemSet(unsigned short selector, unsigned long dest, unsigned char char_to_set, size_t bytes)
{
	unsigned long addr = dest;
	size_t i;

	_farsetsel(selector);

	for (i = 0; i < bytes; i++, addr++) {
		_farnspokeb(addr, char_to_set);
	}
}

void fb_hFarMemSetW(unsigned short selector, unsigned long dest, unsigned short word_to_set, size_t words)
{
	unsigned long addr = dest;
	size_t i;

	_farsetsel(selector);

	for (i = 0; i < words; i++, addr += 2) {
		_farnspokew(addr, word_to_set);
	}
}

/* end of drivers/dos/farmemset.c */
