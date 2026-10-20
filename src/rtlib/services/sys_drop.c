/*
 * Project: FreeBASIC graphics services: sys_drop.c
 * Purpose: Retain bounded drop payloads and decode local file URI lists.
 * Responsibilities: Atomic validation, copied ownership and strict URI decoding.
 * This file intentionally does NOT contain: code to open dropped paths or interpret dropped text as commands.
 * File: services/sys_drop.c
 */
#include "fb_drop.h"

void fb_hDropClear(FB_DROP_STATE *state)
{
	unsigned int index;
	for (index = 0; index < FB_DROP_SLOTS; ++index) {
		free(state->items[index].data);
		memset(&state->items[index], 0, sizeof(state->items[index]));
	}
	state->next_slot = 0;
	/* Keep identifiers monotonic through disable/re-enable within one mode. */
}

int fb_hDropPublish(FB_DROP_STATE *state, int kind, const char *data,
	size_t bytes, int count, EVENT *event)
{
	FB_DROP_ITEM pending;
	size_t offset = 0;
	int index;
	if (!state->supported || !state->enabled)
		return FB_DROP_UNAVAILABLE;
	if (data == NULL || event == NULL || bytes == 0 ||
	    bytes > FB_DROP_MAX_BYTES || count <= 0 ||
	    (unsigned int)count > FB_DROP_MAX_ITEMS ||
	    (kind != FB_DROP_FILES && kind != FB_DROP_TEXT) ||
	    (kind == FB_DROP_TEXT && count != 1) || state->last_identifier == INT_MAX)
		return FB_DROP_INVALID;
	memset(&pending, 0, sizeof(pending));
	for (index = 0; index < count; ++index) {
		const char *terminator;
		size_t length;
		if (offset >= bytes)
			return FB_DROP_INVALID;
		terminator = memchr(data + offset, 0, bytes - offset);
		if (terminator == NULL)
			return FB_DROP_INVALID;
		length = (size_t)(terminator - (data + offset));
		if ((kind == FB_DROP_FILES && length == 0) ||
		    !fb_hServiceUtf8(data + offset, length))
			return FB_DROP_INVALID;
		pending.offsets[index] = (uint32_t)offset;
		offset += length + 1u;
	}
	if (offset != bytes)
		return FB_DROP_INVALID;
	pending.data = malloc(bytes);
	if (pending.data == NULL)
		return FB_DROP_FAILED;
	memcpy(pending.data, data, bytes);
	pending.count = count;
	pending.kind = kind;
	pending.identifier = ++state->last_identifier;
	free(state->items[state->next_slot].data);
	state->items[state->next_slot] = pending;
	state->next_slot = (state->next_slot + 1u) % FB_DROP_SLOTS;
	memset(event, 0, sizeof(*event));
	event->type = EVENT_DROP;
	event->drop_id = pending.identifier;
	event->drop_count = count;
	event->drop_kind = kind;
	return FB_DROP_OK;
}

int fb_hDropCopy(FB_DROP_STATE *state, int identifier, int index, char **text)
{
	unsigned int slot;
	if (identifier <= 0 || index < 0 || text == NULL)
		return FB_DROP_INVALID;
	*text = NULL;
	for (slot = 0; slot < FB_DROP_SLOTS; ++slot) {
		FB_DROP_ITEM *item = &state->items[slot];
		if (item->identifier == identifier && item->data != NULL) {
			size_t length;
			if (index >= item->count)
				return FB_DROP_INVALID;
			length = strlen(item->data + item->offsets[index]) + 1u;
			*text = malloc(length);
			if (*text == NULL)
				return FB_DROP_FAILED;
			memcpy(*text, item->data + item->offsets[index], length);
			return FB_DROP_OK;
		}
	}
	return FB_DROP_EXPIRED;
}

static int hex_digit(unsigned char value)
{
	if (value >= '0' && value <= '9') return value - '0';
	if (value >= 'a' && value <= 'f') return value - 'a' + 10;
	if (value >= 'A' && value <= 'F') return value - 'A' + 10;
	return -1;
}

int fb_hDropUriList(const char *source, size_t length, char *output,
	size_t capacity, size_t *bytes, int *count)
{
	size_t position = 0, used = 0;
	int files = 0;
	if (source == NULL || output == NULL || bytes == NULL || count == NULL ||
	    length > FB_DROP_MAX_BYTES || capacity > FB_DROP_MAX_BYTES)
		return FB_DROP_INVALID;
	*bytes = 0;
	*count = 0;
	while (position < length) {
		size_t begin = position, end, path_begin;
		while (position < length && source[position] != '\n') ++position;
		end = position;
		if (position < length) ++position;
		if (end > begin && source[end - 1u] == '\r') --end;
		if (begin == end || source[begin] == '#') continue;
		if ((unsigned int)files == FB_DROP_MAX_ITEMS || end - begin < 8u ||
		    memcmp(source + begin, "file://", 7u) != 0)
			return FB_DROP_INVALID;
		begin += 7u;
		if (end - begin >= 10u && memcmp(source + begin, "localhost/", 10u) == 0)
			begin += 9u;
		/* Remote hosts, URI queries/fragments and relative paths are not file
		 * drops. Accept only local absolute paths; never fetch a remote URI.
		 */
		if (begin == end || source[begin] != '/') return FB_DROP_INVALID;
		path_begin = used;
		while (begin < end) {
			unsigned char value = (unsigned char)source[begin++];
			if (value == '?' || value == '#') return FB_DROP_INVALID;
			if (value == '%') {
				int high, low;
				if (end - begin < 2u) return FB_DROP_INVALID;
				high = hex_digit((unsigned char)source[begin++]);
				low = hex_digit((unsigned char)source[begin++]);
				if (high < 0 || low < 0) return FB_DROP_INVALID;
				value = (unsigned char)((high << 4) | low);
			}
			if (value < 32u || value == 127u || used >= capacity)
				return FB_DROP_INVALID;
			output[used++] = (char)value;
		}
		if (used >= capacity || !fb_hServiceUtf8(output + path_begin, used - path_begin))
			return FB_DROP_INVALID;
		output[used++] = 0;
		++files;
	}
	if (files == 0) return FB_DROP_INVALID;
	*bytes = used;
	*count = files;
	return FB_DROP_OK;
}

/* end of services/sys_drop.c */
