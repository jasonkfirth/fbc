/*
 * Project: FreeBASIC runtime: sys_hnotify.c
 * Purpose: Refuse notification requests on targets without a native adapter.
 * Responsibilities: Keep the portable ABI linkable with an explicit status.
 * This file intentionally does NOT contain: code to simulate a notification with a console or modal dialog.
 * File: services/sys_hnotify.c
 */
#include "fb_services.h"
int fb_hNotifyCreate(const char *app_name, void **context)
{
	(void)app_name; (void)context;
	return FB_SERVICE_UNAVAILABLE;
}
int fb_hNotifySend(void *context, const char *title, const char *body)
{
	(void)context; (void)title; (void)body;
	return FB_SERVICE_UNAVAILABLE;
}
int fb_hNotifyPoll(void *context)
{
	(void)context;
	return FB_SERVICE_UNAVAILABLE;
}
int fb_hNotifyClose(void *context)
{
	(void)context;
	return FB_SERVICE_UNAVAILABLE;
}
int fb_hNotifyDestroy(void *context)
{
	(void)context;
	return FB_SERVICE_UNAVAILABLE;
}
int fb_hNotifyPermission(void *context, int request)
{
	(void)context; (void)request;
	return FB_SERVICE_UNAVAILABLE;
}

/* end of services/sys_hnotify.c */
