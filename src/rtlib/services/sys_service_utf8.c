/*
 * Project: FreeBASIC runtime: sys_service_utf8.c
 * Purpose: Validate counted UTF-8 passed to native desktop services.
 * Responsibilities: Reject overlong encodings, surrogates, NUL and bad ranges.
 * This file intentionally does NOT contain: code to convert text or access platform services.
 * File: services/sys_service_utf8.c
 */
#include "fb_services.h"

int fb_hServiceUtf8(const char *text, size_t length)
{
	size_t offset = 0;
	if (text == NULL)
		return length == 0;
	while (offset < length) {
		unsigned char lead = (unsigned char)text[offset++];
		uint32_t value;
		uint32_t minimum;
		unsigned int count;
		if (lead == 0)
			return FALSE;
		if (lead < 0x80u)
			continue;
		if (lead >= 0xc2u && lead <= 0xdfu) {
			value = lead & 0x1fu;
			minimum = 0x80u;
			count = 1;
		} else if (lead >= 0xe0u && lead <= 0xefu) {
			value = lead & 0x0fu;
			minimum = 0x800u;
			count = 2;
		} else if (lead >= 0xf0u && lead <= 0xf4u) {
			value = lead & 0x07u;
			minimum = 0x10000u;
			count = 3;
		} else {
			return FALSE;
		}
		if (length - offset < count)
			return FALSE;
		while (count-- != 0) {
			unsigned char next = (unsigned char)text[offset++];
			if ((next & 0xc0u) != 0x80u)
				return FALSE;
			value = (value << 6) | (next & 0x3fu);
		}
		if (value < minimum || value > 0x10ffffu ||
		    (value >= 0xd800u && value <= 0xdfffu))
			return FALSE;
	}
	return TRUE;
}

/* end of services/sys_service_utf8.c */
