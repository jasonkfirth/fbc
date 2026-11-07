/*
 * Project: FreeBASIC Linux graphics tests
 * --------------------------------------
 *
 * File: linux-maliit-events.c
 * Purpose: Check Maliit text and editing keys reach both gfxlib2 queues.
 * Responsibilities: Distinguish Unicode text from encoded extended keys.
 * This file intentionally does NOT connect to a desktop or input method.
 *
 * Build and run on Linux:
 *   cc -std=gnu11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
 *      -pthread tests/gfx/linux-maliit-events.c -Wl,--gc-sections -ldl -lm \
 *      -o /tmp/linux-maliit-events
 *   /tmp/linux-maliit-events
 */
#define DISABLE_X11
#include "../../src/gfxlib2/drivers/linux/gfx_mobile.c"

FBGFX *__fb_gfx;
static int keys[8];
static EVENT events[16];
static int key_count, event_count, lock_count, unlocked_posts;

static void driver_lock(void)
{
	lock_count++;
}

static void driver_unlock(void)
{
	lock_count--;
}

void fb_hPostKey(int key)
{
	if (lock_count != 1)
		unlocked_posts++;
	if (key_count < (int)(sizeof(keys) / sizeof(keys[0])))
		keys[key_count++] = key;
}

void fb_hPostEvent(EVENT *event)
{
	if (lock_count != 1)
		unlocked_posts++;
	if (event_count < (int)(sizeof(events) / sizeof(events[0])))
		events[event_count++] = *event;
}

static void require(int condition, const char *message)
{
	if (!condition) {
		fprintf(stderr, "linux-maliit-events: %s\n", message);
		exit(EXIT_FAILURE);
	}
}

int main(void)
{
	FBGFX gfx;
	GFXDRIVER driver;
	memset(&gfx, 0, sizeof(gfx));
	memset(&driver, 0, sizeof(driver));
	driver.lock = driver_lock;
	driver.unlock = driver_unlock;
	gfx.driver = &driver;
	__fb_gfx = &gfx;
	linux_maliit_post_key(NULL, 'a', FB_LINUX_MALIIT_TEXT);
	linux_maliit_post_key(NULL, 0x4bffu, FB_LINUX_MALIIT_TEXT);
	linux_maliit_post_key(NULL, 0x4bffu, FB_LINUX_MALIIT_KEY);
	linux_maliit_post_key(NULL, '\r', FB_LINUX_MALIIT_KEY);
	linux_maliit_post_key(NULL, '\t', FB_LINUX_MALIIT_TEXT);
	require(lock_count == 0 && unlocked_posts == 0, "driver lock use differs");
	require(key_count == 4 && keys[0] == 'a' && keys[1] == 0x4bff &&
		keys[2] == '\r' && keys[3] == '\t', "legacy keys differ");
	require(event_count == 10, "missing press or release events");
	require(events[0].type == EVENT_KEY_PRESS && events[0].ascii == 'a' &&
		events[1].type == EVENT_KEY_RELEASE, "text event differs");
	require(events[2].ascii == 0x4bff && events[2].scancode == 0,
		"Unicode text was treated as an extended key");
	require(events[4].ascii == 0 && events[4].scancode == SC_LEFT,
		"extended key was treated as Unicode text");
	require(events[6].ascii == '\r' && events[6].scancode == SC_ENTER &&
		events[8].ascii == '\t' && events[8].scancode == SC_TAB,
		"editing key scan codes differ");
	puts("LINUX_MALIIT_EVENTS_PASS");
	return EXIT_SUCCESS;
}

/* end of linux-maliit-events.c */
