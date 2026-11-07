/*
 * FreeBASIC Linux graphics
 * ------------------------
 *
 * File: fb_linux_maliit.h
 *
 * Purpose:
 *
 *     Define the optional Maliit on-screen keyboard bridge shared by Linux
 *     graphics backends.
 *
 * Responsibilities:
 *
 *     - request keyboard visibility through a focused peer connection
 *     - publish the active text cursor area
 *     - deliver committed text and editing keys to the graphics input queue
 *
 * This file intentionally does NOT contain:
 *
 *     - Linux device classification or window management
 *     - a dependency on Maliit or GLib development headers
 *     - text widget editing policy
 */

#ifndef FB_LINUX_MALIIT_H
#define FB_LINUX_MALIIT_H

#include <stdint.h>

typedef enum FB_LINUX_MALIIT_INPUT_KIND {
	FB_LINUX_MALIIT_TEXT,
	FB_LINUX_MALIIT_KEY
} FB_LINUX_MALIIT_INPUT_KIND;

/* The bridge serializes callback delivery with mode shutdown. */
typedef void (*FB_LINUX_MALIIT_KEY_CALLBACK)(void *user_data, uint32_t key,
	FB_LINUX_MALIIT_INPUT_KIND kind);

/* A fallback runs on the request worker and must not depend on a live mode. */
typedef void (*FB_LINUX_MALIIT_FALLBACK_CALLBACK)(int visible);

/*
 * Gfxlib's legacy extended keys carry the PC scan code in their upper byte.
 * The four control characters use the corresponding fb_event.h scan codes.
 * Call this for a text codepoint only when it fits in one byte: an arbitrary
 * Unicode codepoint can otherwise look like an encoded extended key.
 */
static inline int fb_hLinuxMaliitKeyScancode(uint32_t key)
{
	switch (key) {
	case 27: return 0x01; /* SC_ESCAPE */
	case '\b': return 0x0e; /* SC_BACKSPACE */
	case '\t': return 0x0f; /* SC_TAB */
	case '\n':
	case '\r': return 0x1c; /* SC_ENTER */
	default:
		if (key > 0xffu && key <= 0x7fffu && (key & 0xffu) == 0xffu)
			return (int)(key >> 8);
		return 0;
	}
}

void fb_hLinuxMaliitSetKeyCallback(FB_LINUX_MALIIT_KEY_CALLBACK callback,
	void *user_data);
void fb_hLinuxMaliitSetTextRegion(int x, int y, int width, int height,
	uint64_t window_id);
void fb_hLinuxMaliitSetWindowId(uint64_t window_id);
int fb_hLinuxMaliitRequestKeyboard(int visible);
int fb_hLinuxMaliitQueueKeyboard(int visible,
	FB_LINUX_MALIIT_FALLBACK_CALLBACK fallback);
void fb_hLinuxMaliitCancelPending(void);
int fb_hLinuxMaliitQueryKeyboard(int *visible);

/* Clip the current peer's keyboard area to its drawable. No bus calls are
 * made here; unknown or inactive bounds leave both outputs unchanged. */
int fb_hLinuxMaliitGetKeyboardOcclusion(int width, int height,
	int *top, int *overlap);

/* Query KWin on the caller's worker thread using the shared GIO binding. */
int fb_hLinuxKwinQueryKeyboard(int *visible);

#endif

/* end of fb_linux_maliit.h */
