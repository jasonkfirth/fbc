/*
 * Project: FreeBASIC X11 graphics input
 * File: fb_x11_key_text.h
 * Purpose: Decode one character returned by XLookupString.
 * Responsibilities: Preserve legacy single bytes and validate UTF-8 scalars.
 * This file does not query X11, post events, or manage input methods.
 *
 * In a UTF-8 locale XLookupString can return several bytes for one key.
 * In a single-byte locale its one-byte result retains the historical value.
 * Longer results must contain exactly one valid Unicode scalar; unsupported
 * or malformed strings leave the caller free to use the legacy scan code.
 */
#ifndef FB_X11_KEY_TEXT_H
#define FB_X11_KEY_TEXT_H

#include <stddef.h>
#include <stdint.h>

static inline uint32_t fb_hX11DecodeKeyText(const char *text, size_t length)
{
	const unsigned char *bytes = (const unsigned char *)text;
	uint32_t value;

	if (!bytes || !length)
		return 0u;
	if (length == 1u)
		return bytes[0];
	if ((length == 2u) && (bytes[0] >= 0xC2u) && (bytes[0] <= 0xDFu) &&
	    ((bytes[1] & 0xC0u) == 0x80u)) {
		return ((uint32_t)(bytes[0] & 0x1Fu) << 6) |
			(uint32_t)(bytes[1] & 0x3Fu);
	}
	if ((length == 3u) && (bytes[0] >= 0xE0u) && (bytes[0] <= 0xEFu) &&
	    ((bytes[1] & 0xC0u) == 0x80u) && ((bytes[2] & 0xC0u) == 0x80u) &&
	    !((bytes[0] == 0xE0u) && (bytes[1] < 0xA0u)) &&
	    !((bytes[0] == 0xEDu) && (bytes[1] >= 0xA0u))) {
		return ((uint32_t)(bytes[0] & 0x0Fu) << 12) |
			((uint32_t)(bytes[1] & 0x3Fu) << 6) |
			(uint32_t)(bytes[2] & 0x3Fu);
	}
	if ((length == 4u) && (bytes[0] >= 0xF0u) && (bytes[0] <= 0xF4u) &&
	    ((bytes[1] & 0xC0u) == 0x80u) && ((bytes[2] & 0xC0u) == 0x80u) &&
	    ((bytes[3] & 0xC0u) == 0x80u) &&
	    !((bytes[0] == 0xF0u) && (bytes[1] < 0x90u)) &&
	    !((bytes[0] == 0xF4u) && (bytes[1] >= 0x90u))) {
		value = ((uint32_t)(bytes[0] & 0x07u) << 18) |
			((uint32_t)(bytes[1] & 0x3Fu) << 12) |
			((uint32_t)(bytes[2] & 0x3Fu) << 6) |
			(uint32_t)(bytes[3] & 0x3Fu);
		return value;
	}
	return 0u;
}

#endif

/* end of fb_x11_key_text.h */
