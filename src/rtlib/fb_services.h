/*
 * Project: FreeBASIC runtime: fb_services.h
 * Purpose: Define checked notification and cooperative idle-loop contracts.
 * Responsibilities: Shared status values, public ABI and platform hooks.
 * Native UI code, scheduling policy and network clients belong elsewhere.
 * File: fb_services.h
 * This file intentionally does NOT contain: unrelated runtime subsystems.
 */
#ifndef FB_SERVICES_H
#define FB_SERVICES_H

#include "fb.h"
#include <stdint.h>
#include <limits.h>

enum {
	FB_SERVICE_OK = 0,
	FB_SERVICE_UNAVAILABLE = 1,
	FB_SERVICE_INVALID = 2,
	FB_SERVICE_FAILED = 3,
	FB_SERVICE_DENIED = 4,
	FB_SERVICE_WRONG_THREAD = 5
};

/* UTF-8 byte limits, excluding the terminating NUL. No silent truncation. */
#define FB_NOTIFY_APP_MAX 127u
#define FB_NOTIFY_TITLE_MAX 63u
#define FB_NOTIFY_BODY_MAX 255u

typedef struct FB_IDLE_STATE {
	double deadline;
	double previous;
	uint32_t interval_ms;
	int running;
	int busy;
	int reserved;
} FB_IDLE_STATE;

typedef int (FBCALL *FB_IDLE_CALLBACK)(void *user);

#ifdef __cplusplus
extern "C" {
#endif

FBCALL int fb_IdleInit(FB_IDLE_STATE *state, uint32_t interval_ms);
FBCALL int fb_IdleStep(FB_IDLE_STATE *state, FB_IDLE_CALLBACK callback,
	void *user, int maximum_wait_ms);
FBCALL int fb_IdleStop(FB_IDLE_STATE *state);
int fb_hIdleNow(double *seconds);

FBCALL int fb_NotifyCreate(FBSTRING *app_name, void **context);
FBCALL int fb_NotifySend(void *context, FBSTRING *title, FBSTRING *body);
FBCALL int fb_NotifyPoll(void *context);
FBCALL int fb_NotifyClose(void *context);
FBCALL int fb_NotifyDestroy(void **context);
FBCALL int fb_NotifyPermission(void *context, int request);

int fb_hNotifyCreate(const char *app_name, void **context);
int fb_hNotifySend(void *context, const char *title, const char *body);
int fb_hNotifyPoll(void *context);
int fb_hNotifyClose(void *context);
int fb_hNotifyDestroy(void *context);
int fb_hNotifyPermission(void *context, int request);

/* Validate exactly length bytes. Embedded NUL and malformed UTF-8 are refused. */
int fb_hServiceUtf8(const char *text, size_t length);

#ifdef __cplusplus
}
#endif

#endif

/* end of fb_services.h */
