/*
 * Project: FreeBASIC graphics services: fb_drop.h
 * Purpose: Describe bounded, copied drag-and-drop payloads beside SCREENEVENT.
 * Responsibilities: Shared queue ABI, limits and platform-independent parsing.
 * Native window protocols, input locks and file opening belong elsewhere.
 * File: fb_drop.h
 * This file intentionally does NOT contain: unrelated runtime subsystems.
 */
#ifndef FB_DROP_H
#define FB_DROP_H
#include "fb_services.h"

#define FB_DROP_FILES 1
#define FB_DROP_TEXT 2
#define FB_DROP_SLOTS 8u
#define FB_DROP_MAX_ITEMS 64u
#define FB_DROP_MAX_BYTES 65536u
enum {
	FB_DROP_OK = 0,
	FB_DROP_UNAVAILABLE = 1,
	FB_DROP_INVALID = 2,
	FB_DROP_FAILED = 3,
	FB_DROP_EXPIRED = 4
};

typedef struct FB_DROP_ITEM {
	char *data;
	uint32_t offsets[FB_DROP_MAX_ITEMS];
	int identifier;
	int count;
	int kind;
} FB_DROP_ITEM;

/* Every access, including supported/enabled, holds the containing input lock.
 * A mode owns at most eight payloads, each at most 64 KiB including terminators.
 * No pointer to native selection memory is retained past event processing.
 */
typedef struct FB_DROP_STATE {
	FB_DROP_ITEM items[FB_DROP_SLOTS];
	unsigned int next_slot;
	int last_identifier;
	int supported;
	int enabled;
} FB_DROP_STATE;

#ifdef __cplusplus
extern "C" {
#endif

void fb_hDropClear(FB_DROP_STATE *state);
int fb_hDropPublish(FB_DROP_STATE *state, int kind, const char *data,
	size_t bytes, int count, EVENT *event);
int fb_hDropCopy(FB_DROP_STATE *state, int identifier, int index, char **text);
int fb_hDropUriList(const char *source, size_t length, char *output,
	size_t capacity, size_t *bytes, int *count);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_drop.h */
