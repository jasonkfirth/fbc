/* FreeBASIC Android tests: keymap.c.
 * Purpose: Check editor/navigation keys against the PC scan-code contract.
 * Responsibilities: Compile the exact production translator and reject unknowns.
 * This file does not create windows, inject input or depend on a keyboard layout. */
#include "fb.h"
#include <android/keycodes.h>
#include <assert.h>
#include <stdio.h>
#include "keymap-under-test.h"

int main(void)
{
	assert(android_key_to_scancode(AKEYCODE_A) == 0x1e);
	assert(android_key_to_scancode(AKEYCODE_B) == 0x30);
	assert(android_key_to_scancode(AKEYCODE_C) == 0x2e);
	assert(android_key_to_scancode(AKEYCODE_K) == 0x25);
	assert(android_key_to_scancode(AKEYCODE_Q) == 0x10);
	assert(android_key_to_scancode(AKEYCODE_Z) == 0x2c);
	assert(android_key_to_scancode(AKEYCODE_1) == SC_1);
	assert(android_key_to_scancode(AKEYCODE_9) == SC_9);
	assert(android_key_to_scancode(AKEYCODE_0) == SC_0);
	assert(android_key_to_scancode(AKEYCODE_MOVE_HOME) == SC_HOME);
	assert(android_key_to_scancode(AKEYCODE_MOVE_END) == SC_END);
	assert(android_key_to_scancode(AKEYCODE_FORWARD_DEL) == SC_DELETE);
	assert(android_key_to_scancode(AKEYCODE_SHIFT_LEFT) == SC_LSHIFT);
	assert(android_key_to_scancode(AKEYCODE_SHIFT_RIGHT) == SC_RSHIFT);
	assert(android_key_to_scancode(AKEYCODE_CTRL_LEFT) == SC_CONTROL);
	assert(android_key_to_scancode(-1) == 0);
	assert(android_key_to_scancode(0x7fffffff) == 0);
	puts("ANDROID_KEYMAP=PASS checks=17");
	return 0;
}
/* end of keymap.c */
