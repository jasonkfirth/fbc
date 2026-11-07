/*
 * Project: FreeBASIC graphics tests
 * File: x11-key-text.c
 * Purpose: Exercise the shared X11 key-text decoder.
 * Responsibilities: Check legacy bytes, UTF-8 scalars, and invalid sequences.
 * This file does not create an X11 window or test the event queue.
 */
#include <assert.h>
#include <stdint.h>
#include "../../src/gfxlib2/drivers/unix/fb_x11_key_text.h"

int main(void)
{
	assert(fb_hX11DecodeKeyText(NULL, 1) == 0u);
	assert(fb_hX11DecodeKeyText("", 0) == 0u);
	assert(fb_hX11DecodeKeyText("a", 1) == 0x61u);
	assert(fb_hX11DecodeKeyText("\xE4", 1) == 0xE4u);
	assert(fb_hX11DecodeKeyText("\x7F", 1) == 0x7Fu);
	assert(fb_hX11DecodeKeyText("\xC3\xA4", 2) == 0xE4u);
	assert(fb_hX11DecodeKeyText("\xE4\xB8\xAD", 3) == 0x4E2Du);
	assert(fb_hX11DecodeKeyText("\xF0\x9F\x98\x80", 4) == 0x1F600u);
	assert(fb_hX11DecodeKeyText("\xF4\x8F\xBF\xBF", 4) == 0x10FFFFu);
	assert(fb_hX11DecodeKeyText("\xC0\x80", 2) == 0u);
	assert(fb_hX11DecodeKeyText("\xE0\x80\x80", 3) == 0u);
	assert(fb_hX11DecodeKeyText("\xED\xA0\x80", 3) == 0u);
	assert(fb_hX11DecodeKeyText("\xF0\x80\x80\x80", 4) == 0u);
	assert(fb_hX11DecodeKeyText("\xF4\x90\x80\x80", 4) == 0u);
	assert(fb_hX11DecodeKeyText("\xF5\x80\x80\x80", 4) == 0u);
	assert(fb_hX11DecodeKeyText("\xE4\xB8", 2) == 0u);
	assert(fb_hX11DecodeKeyText("\xC3x", 2) == 0u);
	assert(fb_hX11DecodeKeyText("ab", 2) == 0u);
	assert(fb_hX11DecodeKeyText("\xF0\x9F\x98\x80x", 5) == 0u);
	return 0;
}

/* end of x11-key-text.c */
