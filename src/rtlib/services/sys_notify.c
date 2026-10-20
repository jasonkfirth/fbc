/*
 * Project: FreeBASIC runtime: sys_notify.c
 * Purpose: Validate notifications before crossing native service boundaries.
 * Responsibilities: Counted UTF-8 strings, bounds and checked context outputs.
 * Native handles and notification policy are owned by platform adapters.
 * File: services/sys_notify.c
 * This file intentionally does NOT contain: unrelated runtime subsystems.
 */
#include "fb_services.h"

static int valid_string(const FBSTRING *text, size_t maximum, int allow_empty)
{
	return text != NULL && (size_t)FB_STRSIZE(text) <= maximum &&
		(allow_empty || FB_STRSIZE(text) != 0) &&
		fb_hServiceUtf8(text->data, (size_t)FB_STRSIZE(text));
}

FBCALL int fb_NotifyCreate(FBSTRING *app_name, void **context)
{
	int result = FB_SERVICE_INVALID;
	if (context != NULL && *context == NULL &&
	    valid_string(app_name, FB_NOTIFY_APP_MAX, FALSE))
		result = fb_hNotifyCreate(app_name->data, context);
	fb_hStrDelTemp(app_name);
	return result;
}

FBCALL int fb_NotifySend(void *context, FBSTRING *title, FBSTRING *body)
{
	int result = FB_SERVICE_INVALID;
	if (context != NULL && valid_string(title, FB_NOTIFY_TITLE_MAX, FALSE) &&
	    valid_string(body, FB_NOTIFY_BODY_MAX, TRUE))
		result = fb_hNotifySend(context, title->data,
			FB_STRSIZE(body) != 0 ? body->data : "");
	fb_hStrDelTemp(title);
	fb_hStrDelTemp(body);
	return result;
}

FBCALL int fb_NotifyPoll(void *context)
{
	return context != NULL ? fb_hNotifyPoll(context) : FB_SERVICE_INVALID;
}

FBCALL int fb_NotifyClose(void *context)
{
	return context != NULL ? fb_hNotifyClose(context) : FB_SERVICE_INVALID;
}

FBCALL int fb_NotifyDestroy(void **context)
{
	int result;
	if (context == NULL)
		return FB_SERVICE_INVALID;
	if (*context == NULL)
		return FB_SERVICE_OK;
	result = fb_hNotifyDestroy(*context);
	if (result == FB_SERVICE_OK)
		*context = NULL;
	return result;
}

FBCALL int fb_NotifyPermission(void *context, int request)
{
	if (context == NULL || (request != 0 && request != 1 && request != -1))
		return FB_SERVICE_INVALID;
	return fb_hNotifyPermission(context, request != 0);
}

/* end of services/sys_notify.c */
