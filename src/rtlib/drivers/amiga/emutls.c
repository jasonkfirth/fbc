/*
    Project: FreeBASIC AmigaOS runtime
    ----------------------------------

    File: drivers/amiga/emutls.c

    Purpose:

        Provide per-thread storage for GCC's emulated TLS variables on the
        classic AmigaOS m68k toolchain.

    Responsibilities:

        - keep one aligned copy of each emulated TLS variable per pthread
        - apply the variable's static initializer to each thread's first copy
        - release a thread's emulated TLS storage when that thread exits

    This file intentionally does NOT contain:

        - compiler TLS parsing or code generation
        - the generic FreeBASIC thread context table
        - non-threaded emulated TLS policy
*/

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef ENABLE_MT

/* GCC 6's Amiga emutls helper stores one object pointer in the descriptor.
   That fallback is process-wide, so use the descriptor address as the key in
   a pthread-specific list instead.  The remaining fields match GCC's
   __emutls_register_common layout: size, alignment, object, template. */
typedef struct FB_AMIGA_EMUTLS_CONTROL {
	size_t size;
	size_t alignment;
	void *object;
	const void *template_data;
} FB_AMIGA_EMUTLS_CONTROL;

typedef struct FB_AMIGA_EMUTLS_ENTRY {
	struct FB_AMIGA_EMUTLS_ENTRY *next;
	const FB_AMIGA_EMUTLS_CONTROL *control;
	void *allocation;
	void *value;
} FB_AMIGA_EMUTLS_ENTRY;

typedef struct FB_AMIGA_EMUTLS_CONTEXT {
	FB_AMIGA_EMUTLS_ENTRY *entries;
} FB_AMIGA_EMUTLS_CONTEXT;

static pthread_key_t fb_amiga_emutls_key;
static pthread_once_t fb_amiga_emutls_once = PTHREAD_ONCE_INIT;

static void fb_amiga_emutls_free_context(void *context_pointer)
{
	FB_AMIGA_EMUTLS_CONTEXT *context = context_pointer;
	FB_AMIGA_EMUTLS_ENTRY *entry;

	if (context == NULL)
		return;

	entry = context->entries;
	while (entry != NULL) {
		FB_AMIGA_EMUTLS_ENTRY *next = entry->next;
		free(entry->allocation);
		free(entry);
		entry = next;
	}

	free(context);
}

static void fb_amiga_emutls_create_key(void)
{
	if (pthread_key_create(&fb_amiga_emutls_key,
			       fb_amiga_emutls_free_context) != 0)
		abort();
}

static void *fb_amiga_emutls_create_value(
	const FB_AMIGA_EMUTLS_CONTROL *control,
	void **allocation_out)
{
	size_t size = control->size;
	size_t alignment = control->alignment;
	size_t allocation_size;
	uintptr_t aligned_address;
	void *allocation;
	void *value;

	if (alignment == 0)
		alignment = 1;
	if ((alignment & (alignment - 1)) != 0 ||
	    size > SIZE_MAX - (alignment - 1))
		abort();

	allocation_size = size + alignment - 1;
	if (allocation_size == 0)
		allocation_size = 1;

	allocation = malloc(allocation_size);
	if (allocation == NULL)
		abort();
	*allocation_out = allocation;

	aligned_address = ((uintptr_t)allocation + alignment - 1) &
		~(uintptr_t)(alignment - 1);
	value = (void *)aligned_address;

	if (control->template_data != NULL)
		memcpy(value, control->template_data, size);
	else if (size != 0)
		memset(value, 0, size);

	return value;
}

void *__emutls_get_address(FB_AMIGA_EMUTLS_CONTROL *control)
{
	FB_AMIGA_EMUTLS_CONTEXT *context;
	FB_AMIGA_EMUTLS_ENTRY *entry;
	int status;

	if (control == NULL)
		abort();

	status = pthread_once(&fb_amiga_emutls_once,
			     fb_amiga_emutls_create_key);
	if (status != 0)
		abort();

	context = pthread_getspecific(fb_amiga_emutls_key);
	if (context == NULL) {
		context = calloc(1, sizeof(*context));
		if (context == NULL)
			abort();
		if (pthread_setspecific(fb_amiga_emutls_key, context) != 0) {
			free(context);
			abort();
		}
	}

	for (entry = context->entries; entry != NULL; entry = entry->next) {
		if (entry->control == control)
			return entry->value;
	}

	entry = calloc(1, sizeof(*entry));
	if (entry == NULL)
		abort();
	entry->value = fb_amiga_emutls_create_value(control,
						   &entry->allocation);
	entry->control = control;
	entry->next = context->entries;
	context->entries = entry;

	return entry->value;
}

#endif

/* end of drivers/amiga/emutls.c */
