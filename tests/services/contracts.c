/*
 * FreeBASIC runtime tests: contracts.c
 * Purpose: Exercise failure paths, time control and untrusted drop boundaries.
 * Responsibilities: Deterministic service contracts without native UI access.
 * Live platform adapters and graphical delivery are checked by separate fixtures.
 */
#include "../../src/rtlib/fb_drop.h"
#include <assert.h>
#include <stdio.h>

static double now;
static int clock_ready = TRUE;
static int waits, callbacks, sends;
static FB_IDLE_STATE *callback_state;

int fb_hIdleNow(double *seconds) { *seconds = now; return clock_ready; }
FBCALL void fb_Delay(int milliseconds)
{
	assert(milliseconds > 0 && milliseconds <= 60000);
	++waits;
	now += (double)milliseconds / 1000.0;
}
FBCALL int fb_hStrDelTemp(FBSTRING *text) { (void)text; return 0; }
int fb_hNotifyCreate(const char *app, void **context)
{
	assert(strcmp(app, "test") == 0);
	*context = &sends;
	return FB_SERVICE_OK;
}
int fb_hNotifySend(void *context, const char *title, const char *body)
{
	assert(context == &sends && strcmp(title, "title") == 0);
	assert(strcmp(body, "body") == 0 || body[0] == 0);
	++sends;
	return FB_SERVICE_OK;
}
int fb_hNotifyPoll(void *context) { (void)context; return FB_SERVICE_OK; }
int fb_hNotifyClose(void *context) { (void)context; return FB_SERVICE_OK; }
int fb_hNotifyDestroy(void *context) { (void)context; return FB_SERVICE_OK; }
int fb_hNotifyPermission(void *context, int request)
{
	(void)context; (void)request;
	return FB_SERVICE_OK;
}

static FBCALL int tick(void *user)
{
	int status;
	assert(user == &callbacks);
	++callbacks;
	status = fb_IdleStep(callback_state, tick, user, 0);
	assert(status == FB_SERVICE_INVALID);
	status = fb_IdleInit(callback_state, 10);
	assert(status == FB_SERVICE_INVALID);
	if (callbacks == 2) {
		status = fb_IdleStop(callback_state);
		assert(status == FB_SERVICE_OK);
	}
	now += 3.0; /* A slow callback must not cause catch-up invocations. */
	return 1;
}

static void test_idle(void)
{
	FB_IDLE_STATE state = {0}, saved;
	int status;
	callback_state = &state;
	assert(sizeof(state) == 32);
	assert(fb_IdleInit(&state, 0) == FB_SERVICE_INVALID);
	assert(fb_IdleInit(&state, 86400001u) == FB_SERVICE_INVALID);
	assert(fb_IdleInit(&state, 100) == FB_SERVICE_OK);
	saved = state;
	status = fb_IdleStep(&state, tick, &callbacks, -1);
	assert(status == FB_SERVICE_INVALID);
	assert(memcmp(&state, &saved, sizeof(state)) == 0);
	status = fb_IdleStep(&state, tick, &callbacks, 0);
	assert(status == FB_SERVICE_OK);
	assert(callbacks == 0 && waits == 0);
	status = fb_IdleStep(&state, tick, &callbacks, 1000);
	assert(status == FB_SERVICE_OK);
	assert(callbacks == 1 && waits == 1);
	assert(state.deadline > now);
	now += 1000;
	status = fb_IdleStep(&state, tick, &callbacks, 0);
	assert(status == FB_SERVICE_OK);
	assert(callbacks == 2 && !state.running);
	status = fb_IdleStep(&state, tick, &callbacks, 1000);
	assert(status == FB_SERVICE_OK);
	assert(waits == 1 && callbacks == 2);
	assert(fb_IdleInit(&state, 100) == FB_SERVICE_OK);
	now -= 100;
	status = fb_IdleStep(&state, tick, &callbacks, 0);
	assert(status == FB_SERVICE_OK);
	assert(state.deadline > now && state.deadline - now < 0.101);
	clock_ready = FALSE;
	status = fb_IdleStep(&state, tick, &callbacks, 0);
	assert(status == FB_SERVICE_UNAVAILABLE);
	clock_ready = TRUE;
}

static void test_notify(void)
{
	void *context = NULL;
	FBSTRING app = {(char *)"test", 4, 4};
	FBSTRING title = {(char *)"title", 5, 5};
	FBSTRING body = {(char *)"body", 4, 4};
	FBSTRING invalid = {(char *)"a\0b", 3, 3};
	FBSTRING empty = {NULL, 0, 0};
	assert(fb_NotifyCreate(&invalid, &context) == FB_SERVICE_INVALID && context == NULL);
	assert(fb_NotifyCreate(&app, &context) == FB_SERVICE_OK);
	assert(fb_NotifyCreate(&app, &context) == FB_SERVICE_INVALID);
	assert(fb_NotifySend(NULL, &title, &body) == FB_SERVICE_INVALID);
	assert(fb_NotifySend(context, &title, &invalid) == FB_SERVICE_INVALID);
	assert(fb_NotifySend(context, &empty, &body) == FB_SERVICE_INVALID);
	assert(sends == 0);
	/* Temporary-string length flags must not reject a normal expression. */
	title.len |= FB_TEMPSTRBIT;
	assert(fb_NotifySend(context, &title, &body) == FB_SERVICE_OK);
	assert(fb_NotifySend(context, &title, &empty) == FB_SERVICE_OK);
	assert(fb_NotifyPermission(context, 2) == FB_SERVICE_INVALID);
	assert(fb_NotifyPermission(context, -1) == FB_SERVICE_OK);
	assert(fb_NotifyDestroy(&context) == FB_SERVICE_OK && context == NULL);
	assert(fb_NotifyDestroy(&context) == FB_SERVICE_OK);
	assert(fb_hServiceUtf8("\xf0\x9f\x8c\x8d", 4));
	assert(!fb_hServiceUtf8("\xc0\xaf", 2));
	assert(!fb_hServiceUtf8("\xed\xa0\x80", 3));
	assert(!fb_hServiceUtf8("\xf4\x90\x80\x80", 4));
	assert(!fb_hServiceUtf8("\xf0\x9f", 2));
	assert(!fb_hServiceUtf8("a\0b", 3));
}

static void test_drop(void)
{
	FB_DROP_STATE state = {0};
	EVENT event = {0};
	char *copy = NULL;
	char output[FB_DROP_MAX_BYTES];
	const char uri[] = "# comment\r\nfile:///tmp/a%20b\r\nfile://localhost/tmp/%E2%98%83\n";
	size_t bytes;
	int count, first_id;
	assert(sizeof(EVENT) == 20);
	assert(fb_hDropPublish(&state, FB_DROP_FILES, "one", 4, 1, &event) == FB_DROP_UNAVAILABLE);
	state.supported = state.enabled = TRUE;
	assert(fb_hDropPublish(&state, FB_DROP_FILES, "one", 3, 1, &event) == FB_DROP_INVALID);
	assert(fb_hDropPublish(&state, FB_DROP_FILES, "one\0x", 6, 1, &event) == FB_DROP_INVALID);
	assert(fb_hDropPublish(&state, FB_DROP_FILES, "one\0two", 8, 2, &event) == FB_DROP_OK);
	assert(event.type == EVENT_DROP && event.drop_count == 2 && event.drop_kind == FB_DROP_FILES);
	first_id = event.drop_id;
	assert(fb_hDropCopy(&state, first_id, 1, &copy) == FB_DROP_OK);
	assert(strcmp(copy, "two") == 0);
	free(copy);
	assert(fb_hDropCopy(&state, first_id, 2, &copy) == FB_DROP_INVALID);
	for (unsigned int index = 0; index < FB_DROP_SLOTS; ++index)
		assert(fb_hDropPublish(&state, FB_DROP_TEXT, "text", 5, 1, &event) == FB_DROP_OK);
	assert(fb_hDropCopy(&state, first_id, 0, &copy) == FB_DROP_EXPIRED);
	assert(fb_hDropUriList(uri, strlen(uri), output, sizeof(output), &bytes, &count) == FB_DROP_OK);
	assert(count == 2 && strcmp(output, "/tmp/a b") == 0);
	assert(strcmp(output + strlen(output) + 1, "/tmp/\xe2\x98\x83") == 0);
#define BAD_URI(text) assert(fb_hDropUriList(text, sizeof(text) - 1u, output, sizeof(output), &bytes, &count) == FB_DROP_INVALID)
	BAD_URI("file://remote/tmp/file");
	BAD_URI("https://example.org/file");
	BAD_URI("file:///tmp/%00name");
	BAD_URI("file:///tmp/%xx");
	BAD_URI("file:///tmp/%C0%AF");
	BAD_URI("file:///tmp/a?query");
	BAD_URI("file:///tmp/a#fragment");
	BAD_URI("file:///tmp/a\0b");
#undef BAD_URI
	fb_hDropClear(&state);
	assert(fb_hDropCopy(&state, event.drop_id, 0, &copy) == FB_DROP_EXPIRED);
	state.last_identifier = INT_MAX;
	assert(fb_hDropPublish(&state, FB_DROP_TEXT, "x", 2, 1, &event) == FB_DROP_INVALID);
}

int main(void)
{
	test_idle();
	test_notify();
	test_drop();
	puts("PLATFORM_SERVICE_CONTRACTS=PASS");
	return 0;
}
/* end of contracts.c */
