/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_command.c

    Purpose:

        Implement owned commands, completion waits, and the bounded command
        queue used between FreeBASIC callers and the render thread.

    Responsibilities:

        - validate and allocate complete command records
        - serialize submissions from multiple producer threads
        - apply back-pressure without busy waiting
        - provide orderly close and immediate failure wakeups

    This file intentionally does NOT contain:

        - rendering logic
        - platform thread creation
        - command payload interpretation
*/

#include "gfx3_command.h"

/* ------------------------------------------------------------------------- */
/* Command allocation                                                        */
/* ------------------------------------------------------------------------- */

FB_GFX3_COMMAND *fb_gfx3_command_create(uint32_t type, size_t payload_size)
{
	FB_GFX3_COMMAND *command;
	size_t header_size;
	size_t allocation_size;

	if (type == FB_GFX3_COMMAND_INVALID)
		return NULL;

	header_size = offsetof(FB_GFX3_COMMAND, payload);
	if (fb_gfx3_size_add(header_size, payload_size, &allocation_size) != FB_GFX3_OK)
		return NULL;

	if ((allocation_size > FB_GFX3_COMMAND_MAX_SIZE) ||
	    (allocation_size > UINT32_MAX))
		return NULL;

	command = (FB_GFX3_COMMAND *)calloc(1, allocation_size);
	if (command == NULL)
		return NULL;

	command->type = type;
	command->size = (uint32_t)allocation_size;
	return command;
}

void fb_gfx3_command_destroy(FB_GFX3_COMMAND *command)
{
	if ((command != NULL) &&
	    ((command->flags & FB_GFX3_COMMAND_REUSABLE) != 0u))
		return;
	free(command);
}

size_t fb_gfx3_command_payload_size(const FB_GFX3_COMMAND *command)
{
	size_t header_size = offsetof(FB_GFX3_COMMAND, payload);

	if ((command == NULL) || (command->size < header_size) ||
	    (command->size > FB_GFX3_COMMAND_MAX_SIZE))
		return 0;

	return (size_t)command->size - header_size;
}

/* ------------------------------------------------------------------------- */
/* Synchronous command completion                                            */
/* ------------------------------------------------------------------------- */

int fb_gfx3_completion_init(FB_GFX3_COMPLETION *completion)
{
	if (completion == NULL)
		return FB_GFX3_INVALID;

	memset(completion, 0, sizeof(*completion));
	completion->mutex = fb_MutexCreate();
	if (completion->mutex == NULL)
		return FB_GFX3_OUT_OF_MEMORY;

	completion->condition = fb_CondCreate();
	if (completion->condition == NULL) {
		fb_MutexDestroy(completion->mutex);
		memset(completion, 0, sizeof(*completion));
		return FB_GFX3_OUT_OF_MEMORY;
	}

	return FB_GFX3_OK;
}

void fb_gfx3_completion_destroy(FB_GFX3_COMPLETION *completion)
{
	if (completion == NULL)
		return;

	if (completion->condition != NULL)
		fb_CondDestroy(completion->condition);
	if (completion->mutex != NULL)
		fb_MutexDestroy(completion->mutex);

	memset(completion, 0, sizeof(*completion));
}

/*
	The caller must own the reusable command associated with this completion and
	must have consumed its preceding result. Keeping the mutex and condition
	alive removes two kernel-object allocations from high-frequency control
	requests such as an empty SCREENEVENT poll.
*/
int fb_gfx3_completion_reset(FB_GFX3_COMPLETION *completion)
{
	if ((completion == NULL) || (completion->mutex == NULL) ||
	    (completion->condition == NULL))
		return FB_GFX3_INVALID;
	fb_MutexLock(completion->mutex);
	completion->sequence = 0;
	memset(completion->value, 0, sizeof(completion->value));
	completion->status = FB_GFX3_OK;
	completion->complete = FALSE;
	fb_MutexUnlock(completion->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_completion_finish(FB_GFX3_COMPLETION *completion,
	uint64_t sequence, int status)
{
	if ((completion == NULL) || (completion->mutex == NULL) ||
	    (completion->condition == NULL))
		return FB_GFX3_INVALID;

	fb_MutexLock(completion->mutex);
	if (completion->complete) {
		fb_MutexUnlock(completion->mutex);
		return FB_GFX3_INVALID;
	}

	completion->sequence = sequence;
	completion->status = status;
	completion->complete = TRUE;
	fb_CondBroadcast(completion->condition);
	fb_MutexUnlock(completion->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_completion_wait(FB_GFX3_COMPLETION *completion,
	uint64_t *sequence)
{
	int status;

	if ((completion == NULL) || (completion->mutex == NULL) ||
	    (completion->condition == NULL))
		return FB_GFX3_INVALID;

	fb_MutexLock(completion->mutex);
	while (!completion->complete)
		fb_CondWait(completion->condition, completion->mutex);

	if (sequence != NULL)
		*sequence = completion->sequence;
	status = completion->status;
	fb_MutexUnlock(completion->mutex);
	return status;
}

int fb_gfx3_completion_set_value(FB_GFX3_COMPLETION *completion,
	size_t index, uint64_t value)
{
	if ((completion == NULL) || (completion->mutex == NULL) ||
	    (index >= (sizeof(completion->value) / sizeof(completion->value[0]))))
		return FB_GFX3_INVALID;

	fb_MutexLock(completion->mutex);
	if (completion->complete) {
		fb_MutexUnlock(completion->mutex);
		return FB_GFX3_INVALID;
	}
	completion->value[index] = value;
	fb_MutexUnlock(completion->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_completion_get_value(FB_GFX3_COMPLETION *completion,
	size_t index, uint64_t *value)
{
	if ((completion == NULL) || (completion->mutex == NULL) || (value == NULL) ||
	    (index >= (sizeof(completion->value) / sizeof(completion->value[0]))))
		return FB_GFX3_INVALID;

	fb_MutexLock(completion->mutex);
	if (!completion->complete) {
		fb_MutexUnlock(completion->mutex);
		return FB_GFX3_INVALID;
	}
	*value = completion->value[index];
	fb_MutexUnlock(completion->mutex);
	return FB_GFX3_OK;
}

/* ------------------------------------------------------------------------- */
/* Bounded command queue                                                     */
/* ------------------------------------------------------------------------- */

int fb_gfx3_queue_init(FB_GFX3_COMMAND_QUEUE *queue, size_t capacity)
{
	size_t allocation_size;

	if ((queue == NULL) || (capacity == 0))
		return FB_GFX3_INVALID;

	if (fb_gfx3_size_multiply(capacity, sizeof(*queue->slots),
	    &allocation_size) != FB_GFX3_OK)
		return FB_GFX3_INVALID;

	memset(queue, 0, sizeof(*queue));
	queue->slots = (FB_GFX3_COMMAND **)calloc(1, allocation_size);
	if (queue->slots == NULL)
		return FB_GFX3_OUT_OF_MEMORY;

	queue->mutex = fb_MutexCreate();
	if (queue->mutex == NULL)
		goto fail;

	queue->can_read = fb_CondCreate();
	if (queue->can_read == NULL)
		goto fail;

	queue->can_write = fb_CondCreate();
	if (queue->can_write == NULL)
		goto fail;

	queue->capacity = capacity;
	queue->next_sequence = 1;
	queue->accepting = TRUE;
	return FB_GFX3_OK;

fail:
	if (queue->can_read != NULL)
		fb_CondDestroy(queue->can_read);
	if (queue->mutex != NULL)
		fb_MutexDestroy(queue->mutex);
	free((void *)queue->slots);
	memset(queue, 0, sizeof(*queue));
	return FB_GFX3_OUT_OF_MEMORY;
}

void fb_gfx3_queue_close(FB_GFX3_COMMAND_QUEUE *queue)
{
	if ((queue == NULL) || (queue->mutex == NULL))
		return;

	fb_MutexLock(queue->mutex);
	queue->accepting = FALSE;
	fb_CondBroadcast(queue->can_read);
	fb_CondBroadcast(queue->can_write);
	fb_MutexUnlock(queue->mutex);
}

void fb_gfx3_queue_fail(FB_GFX3_COMMAND_QUEUE *queue, int failure_code)
{
	if ((queue == NULL) || (queue->mutex == NULL))
		return;

	if (failure_code == FB_GFX3_OK)
		failure_code = FB_GFX3_FAILED;

	fb_MutexLock(queue->mutex);
	queue->accepting = FALSE;
	queue->failed = TRUE;
	queue->failure_code = failure_code;
	fb_CondBroadcast(queue->can_read);
	fb_CondBroadcast(queue->can_write);
	fb_MutexUnlock(queue->mutex);
}

static int fb_gfx3_queue_submit_internal(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND *command, uint64_t *sequence, int final)
{
	uint64_t assigned_sequence;

	if ((queue == NULL) || (command == NULL) || (queue->mutex == NULL) ||
	    (command->type == FB_GFX3_COMMAND_INVALID) ||
	    (command->size < offsetof(FB_GFX3_COMMAND, payload)) ||
	    (command->size > FB_GFX3_COMMAND_MAX_SIZE))
		return FB_GFX3_INVALID;

	fb_MutexLock(queue->mutex);
	while ((queue->count == queue->capacity) && queue->accepting &&
	       !queue->failed)
		fb_CondWait(queue->can_write, queue->mutex);

	if (queue->failed) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_FAILED;
	}

	if (!queue->accepting) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_CLOSED;
	}

	if (queue->next_sequence == 0) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_EXHAUSTED;
	}

	assigned_sequence = queue->next_sequence;
	if (queue->next_sequence == UINT64_MAX)
		queue->next_sequence = 0;
	else
		queue->next_sequence++;

	command->sequence = assigned_sequence;
	queue->slots[queue->tail] = command;
	queue->tail++;
	if (queue->tail == queue->capacity)
		queue->tail = 0;
	queue->count++;
	if (final)
		queue->accepting = FALSE;

	if (sequence != NULL)
		*sequence = assigned_sequence;
	fb_CondSignal(queue->can_read);
	if (final)
		fb_CondBroadcast(queue->can_write);
	fb_MutexUnlock(queue->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_queue_submit(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND *command, uint64_t *sequence)
{
	return fb_gfx3_queue_submit_internal(queue, command, sequence, FALSE);
}

int fb_gfx3_queue_submit_many(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND *const *commands, size_t count, uint64_t *sequence)
{
	uint64_t assigned_sequence = 0;
	size_t index;

	if ((queue == NULL) || (commands == NULL) || (count == 0) ||
	    (queue->mutex == NULL) || (count > queue->capacity))
		return FB_GFX3_INVALID;
	for (index = 0; index < count; index++) {
		FB_GFX3_COMMAND *command = commands[index];

		if ((command == NULL) ||
		    (command->type == FB_GFX3_COMMAND_INVALID) ||
		    (command->size < offsetof(FB_GFX3_COMMAND, payload)) ||
		    (command->size > FB_GFX3_COMMAND_MAX_SIZE))
			return FB_GFX3_INVALID;
	}

	fb_MutexLock(queue->mutex);
	while ((queue->count > queue->capacity - count) && queue->accepting &&
	       !queue->failed)
		fb_CondWait(queue->can_write, queue->mutex);
	if (queue->failed) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_FAILED;
	}
	if (!queue->accepting || (queue->next_sequence == 0)) {
		fb_MutexUnlock(queue->mutex);
		return queue->accepting ? FB_GFX3_EXHAUSTED : FB_GFX3_CLOSED;
	}
	if ((uintmax_t)(count - 1u) > UINT64_MAX - queue->next_sequence) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_EXHAUSTED;
	}
	for (index = 0; index < count; index++) {
		FB_GFX3_COMMAND *command = commands[index];

		assigned_sequence = queue->next_sequence;
		queue->next_sequence++;
		command->sequence = assigned_sequence;
		queue->slots[queue->tail] = command;
		queue->tail++;
		if (queue->tail == queue->capacity)
			queue->tail = 0;
		queue->count++;
	}
	if (sequence != NULL)
		*sequence = assigned_sequence;
	fb_CondSignal(queue->can_read);
	fb_MutexUnlock(queue->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_queue_submit_final(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND *command, uint64_t *sequence)
{
	return fb_gfx3_queue_submit_internal(queue, command, sequence, TRUE);
}

int fb_gfx3_queue_pop(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND **command)
{
	if ((queue == NULL) || (command == NULL) || (queue->mutex == NULL))
		return FB_GFX3_INVALID;

	*command = NULL;
	fb_MutexLock(queue->mutex);
	while ((queue->count == 0) && queue->accepting && !queue->failed)
		fb_CondWait(queue->can_read, queue->mutex);

	if (queue->failed) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_FAILED;
	}

	if (queue->count == 0) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_CLOSED;
	}

	*command = queue->slots[queue->head];
	queue->slots[queue->head] = NULL;
	queue->head++;
	if (queue->head == queue->capacity)
		queue->head = 0;
	queue->count--;

	fb_CondSignal(queue->can_write);
	fb_MutexUnlock(queue->mutex);
	return FB_GFX3_OK;
}

int fb_gfx3_queue_try_pop(FB_GFX3_COMMAND_QUEUE *queue,
	FB_GFX3_COMMAND **command)
{
	int result;

	if ((queue == NULL) || (command == NULL) || (queue->mutex == NULL))
		return FB_GFX3_INVALID;
	*command = NULL;
	fb_MutexLock(queue->mutex);
	if (queue->failed) {
		fb_MutexUnlock(queue->mutex);
		return FB_GFX3_FAILED;
	}
	if (queue->count == 0) {
		result = queue->accepting ? FB_GFX3_EXHAUSTED : FB_GFX3_CLOSED;
		fb_MutexUnlock(queue->mutex);
		return result;
	}
	*command = queue->slots[queue->head];
	queue->slots[queue->head] = NULL;
	queue->head++;
	if (queue->head == queue->capacity)
		queue->head = 0;
	queue->count--;
	fb_CondSignal(queue->can_write);
	fb_MutexUnlock(queue->mutex);
	return FB_GFX3_OK;
}

size_t fb_gfx3_queue_discard(FB_GFX3_COMMAND_QUEUE *queue, int status)
{
	FB_GFX3_COMMAND *command;
	size_t discarded = 0;

	if ((queue == NULL) || (queue->mutex == NULL))
		return 0;

	for (;;) {
		fb_MutexLock(queue->mutex);
		if (queue->count == 0) {
			fb_MutexUnlock(queue->mutex);
			break;
		}

		command = queue->slots[queue->head];
		queue->slots[queue->head] = NULL;
		queue->head++;
		if (queue->head == queue->capacity)
			queue->head = 0;
		queue->count--;
		fb_CondSignal(queue->can_write);
		fb_MutexUnlock(queue->mutex);

		/*
			A non-empty queue should always contain a command at its head.
			Keep teardown safe if corrupted state violates that invariant.
		*/
		if (command == NULL)
			continue;
		if (command->completion != NULL)
			fb_gfx3_completion_finish(command->completion,
				command->sequence, status);
		fb_gfx3_command_destroy(command);
		discarded++;
	}

	return discarded;
}

void fb_gfx3_queue_destroy(FB_GFX3_COMMAND_QUEUE *queue)
{
	size_t i;

	if (queue == NULL)
		return;

	/* All producer and consumer threads must be joined before this call. */
	if (queue->slots != NULL) {
		for (i = 0; i < queue->capacity; i++)
			fb_gfx3_command_destroy(queue->slots[i]);
	}

	if (queue->can_write != NULL)
		fb_CondDestroy(queue->can_write);
	if (queue->can_read != NULL)
		fb_CondDestroy(queue->can_read);
	if (queue->mutex != NULL)
		fb_MutexDestroy(queue->mutex);
	free((void *)queue->slots);
	memset(queue, 0, sizeof(*queue));
}

/* end of gfx3_command.c */
