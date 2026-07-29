/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_command.h

    Purpose:

        Define owned render commands, completion objects, and the bounded
        multi-producer command queue shared with the render thread.

    Responsibilities:

        - define command identity and payload ownership
        - assign a total submission order
        - block producers when the bounded queue is full
        - wake all waiters during close or renderer failure

    This file intentionally does NOT contain:

        - GPU command encoding
        - backend-specific payloads
        - resource lookup or destruction
*/

#ifndef __FB_GFX3_COMMAND_H__
#define __FB_GFX3_COMMAND_H__

#include "fb_gfx3.h"

#define FB_GFX3_COMMAND_MAX_SIZE (64u * 1024u * 1024u)

enum FB_GFX3_COMMAND_TYPE {
	FB_GFX3_COMMAND_INVALID = 0,
	FB_GFX3_COMMAND_RENDERER_INIT,
	FB_GFX3_COMMAND_RENDERER_SHUTDOWN,
	FB_GFX3_COMMAND_SURFACE_CREATE,
	FB_GFX3_COMMAND_SURFACE_DESTROY,
	FB_GFX3_COMMAND_SURFACE_UPLOAD,
	FB_GFX3_COMMAND_SURFACE_DOWNLOAD,
	FB_GFX3_COMMAND_READ_PIXEL,
	FB_GFX3_COMMAND_CLEAR,
	FB_GFX3_COMMAND_POINTS,
	FB_GFX3_COMMAND_LINE,
	FB_GFX3_COMMAND_RECTANGLE,
	FB_GFX3_COMMAND_ELLIPSE,
	FB_GFX3_COMMAND_BLIT,
	FB_GFX3_COMMAND_GLYPHS,
	FB_GFX3_COMMAND_PALETTE,
	FB_GFX3_COMMAND_BARRIER,
	FB_GFX3_COMMAND_PAGE_SET,
	FB_GFX3_COMMAND_PRESENT,
	FB_GFX3_COMMAND_WINDOW_TITLE,
	FB_GFX3_COMMAND_PLATFORM_POLL,
	FB_GFX3_COMMAND_INTEROP_CALLBACK,
	/* Append-only: command values are consumed by independently built archives. */
	FB_GFX3_COMMAND_PAINT,
	/* Bounded producer-side packet for adjacent opaque filled rectangles. */
	FB_GFX3_COMMAND_RECTANGLES,
	/* Bounded producer-side packet for adjacent compatible GPU blits. */
	FB_GFX3_COMMAND_BLITS,
	/* Inverse-mapped affine or projective GPU surface operation. */
	FB_GFX3_COMMAND_TRANSFORM_BLIT,
	/* Process native input without performing GPU maintenance or a GPU wait. */
	FB_GFX3_COMMAND_INPUT_POLL,
	/* Bounded producer-side packet for adjacent compatible GPU lines. */
	FB_GFX3_COMMAND_LINES
};

enum FB_GFX3_COMMAND_FLAG {
	/*
		The owner retains this command allocation after renderer execution. This
		is reserved for serialized, payload-free control requests whose lifetime
		is the complete renderer context.
	*/
	FB_GFX3_COMMAND_REUSABLE = 0x00000001u
};

typedef struct FB_GFX3_COMPLETION {
	FBMUTEX *mutex;
	FBCOND *condition;
	uint64_t sequence;
	uint64_t value[4];
	int status;
	int complete;
} FB_GFX3_COMPLETION;

typedef struct FB_GFX3_COMMAND {
	uint32_t type;
	uint32_t size;
	uint32_t flags;
	uint32_t reserved;
	uint64_t sequence;
	FB_GFX3_HANDLE target;
	FB_GFX3_COMPLETION *completion;
	unsigned char payload[];
} FB_GFX3_COMMAND;

typedef struct FB_GFX3_COMMAND_QUEUE {
	FBMUTEX *mutex;
	FBCOND *can_read;
	FBCOND *can_write;
	FB_GFX3_COMMAND **slots;
	size_t capacity;
	size_t head;
	size_t tail;
	size_t count;
	uint64_t next_sequence;
	int accepting;
	int failed;
	int failure_code;
} FB_GFX3_COMMAND_QUEUE;

FB_GFX3_COMMAND *fb_gfx3_command_create(uint32_t type, size_t payload_size);
void fb_gfx3_command_destroy(FB_GFX3_COMMAND *command);
size_t fb_gfx3_command_payload_size(const FB_GFX3_COMMAND *command);

int fb_gfx3_completion_init(FB_GFX3_COMPLETION *completion);
void fb_gfx3_completion_destroy(FB_GFX3_COMPLETION *completion);
int fb_gfx3_completion_reset(FB_GFX3_COMPLETION *completion);
int fb_gfx3_completion_finish(FB_GFX3_COMPLETION *completion,
	uint64_t sequence, int status);
int fb_gfx3_completion_wait(FB_GFX3_COMPLETION *completion,
	uint64_t *sequence);
int fb_gfx3_completion_set_value(FB_GFX3_COMPLETION *completion,
	size_t index, uint64_t value);
int fb_gfx3_completion_get_value(FB_GFX3_COMPLETION *completion,
	size_t index, uint64_t *value);

int fb_gfx3_queue_init(FB_GFX3_COMMAND_QUEUE *queue, size_t capacity);
void fb_gfx3_queue_close(FB_GFX3_COMMAND_QUEUE *queue);
void fb_gfx3_queue_fail(FB_GFX3_COMMAND_QUEUE *queue, int failure_code);
int fb_gfx3_queue_submit(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND *command, uint64_t *sequence);
int fb_gfx3_queue_submit_many(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND *const *commands, size_t count, uint64_t *sequence);
int fb_gfx3_queue_submit_final(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND *command, uint64_t *sequence);
int fb_gfx3_queue_pop(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND **command);
int fb_gfx3_queue_try_pop(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND **command);
size_t fb_gfx3_queue_discard(FB_GFX3_COMMAND_QUEUE *queue, int status);
void fb_gfx3_queue_destroy(FB_GFX3_COMMAND_QUEUE *queue);

#endif

/* end of gfx3_command.h */
