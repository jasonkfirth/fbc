/*
 * Project: FreeBASIC runtime regression tests
 * File: js-exepath-bounds.c
 * Purpose: Check the JavaScript executable-path provider's buffer contract.
 * Responsibilities: Exercise absent, empty, short and sufficient destinations.
 * This file intentionally does NOT start a browser or replace the provider.
 */
#include "fb.h"
#include <assert.h>

int main(void)
{
	char data[3] = { 'X', 'Y', 'Z' };
	char *result;

	result = fb_hGetExePath(NULL, 2);
	assert(result == NULL);
	result = fb_hGetExePath(data, -1);
	assert(result == NULL && data[0] == 'X');
	result = fb_hGetExePath(data, 0);
	assert(result == NULL && data[0] == 'X');
	result = fb_hGetExePath(data, 1);
	assert(result == NULL && data[0] == '\0' && data[1] == 'Y');
	result = fb_hGetExePath(data, 2);
	assert(result == data && data[0] == '/' && data[1] == '\0');
	assert(data[2] == 'Z');
	puts("JS_EXEPATH_BOUNDS_PASS");
	return 0;
}

/* end of js-exepath-bounds.c */
