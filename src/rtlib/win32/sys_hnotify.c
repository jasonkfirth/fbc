/*
 * FreeBASIC Windows runtime: sys_hnotify.c
 * Purpose: Present passive notifications through the Windows shell.
 * Responsibilities: Owner-thread window, UTF-16 conversion and tray lifetime.
 * This file does not create a worker, consume application input or show dialogs.
 */
#include "../fb_services.h"
#include <windows.h>
#include <shellapi.h>

typedef BOOL (WINAPI *FB_SHELL_NOTIFY)(DWORD, PNOTIFYICONDATAW);
typedef struct FB_NOTIFY_WIN32 {
	DWORD owner;
	HMODULE library;
	FB_SHELL_NOTIFY shell_notify;
	NOTIFYICONDATAW icon;
	int added;
} FB_NOTIFY_WIN32;

static int on_owner(FB_NOTIFY_WIN32 *context)
{
	return context->owner == GetCurrentThreadId();
}

int fb_hNotifyCreate(const char *app_name, void **output)
{
	FB_NOTIFY_WIN32 *context = calloc(1, sizeof(*context));
	wchar_t path[MAX_PATH];
	UINT length;
	FARPROC address;
	if (context == NULL)
		return FB_SERVICE_FAILED;
	/* An absolute system path prevents a current-directory DLL substitution. */
	length = GetSystemDirectoryW(path, MAX_PATH);
	if (length == 0 || length >= MAX_PATH - 13u)
		goto unavailable;
	memcpy(path + length, L"\\shell32.dll", sizeof(L"\\shell32.dll"));
	context->library = LoadLibraryW(path);
	if (context->library == NULL)
		goto unavailable;
	address = GetProcAddress(context->library, "Shell_NotifyIconW");
	if (address == NULL)
		goto unavailable;
	_Static_assert(sizeof(address) == sizeof(context->shell_notify),
		"Win32 function pointer width");
	memcpy(&context->shell_notify, &address, sizeof(address));
	context->owner = GetCurrentThreadId();
	context->icon.cbSize = sizeof(context->icon);
	context->icon.uID = 1;
	context->icon.hIcon = LoadIconW(NULL, MAKEINTRESOURCEW(32512));
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, app_name, -1,
		context->icon.szTip, 128))
		goto unavailable;
	/* A private hidden window keeps the shell icon alive until explicit
	 * teardown. The built-in STATIC class needs no shared registration state.
	 */
	context->icon.hWnd = CreateWindowExW(0, L"STATIC", L"FreeBASIC notifications",
		0, 0, 0, 0, 0, NULL, NULL, GetModuleHandleW(NULL), NULL);
	if (context->icon.hWnd == NULL)
		goto unavailable;
	*output = context;
	return FB_SERVICE_OK;
unavailable:
	if (context->library != NULL)
		FreeLibrary(context->library);
	free(context);
	return FB_SERVICE_UNAVAILABLE;
}

int fb_hNotifySend(void *opaque, const char *title, const char *body)
{
	FB_NOTIFY_WIN32 *context = opaque;
	if (!on_owner(context))
		return FB_SERVICE_WRONG_THREAD;
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, title, -1,
		context->icon.szInfoTitle, 64) ||
	    !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, body, -1,
		context->icon.szInfo, 256))
		return FB_SERVICE_INVALID;
	if (!context->added) {
		context->icon.uFlags = NIF_ICON | NIF_TIP;
		if (!context->shell_notify(NIM_ADD, &context->icon))
			return FB_SERVICE_UNAVAILABLE;
		context->added = TRUE;
	}
	context->icon.uFlags = NIF_INFO;
	context->icon.dwInfoFlags = NIIF_INFO;
	if (!context->shell_notify(NIM_MODIFY, &context->icon)) {
		/* Explorer may have restarted since the previous send. Re-register
		 * this context once, without leaving duplicate icons behind.
		 */
		context->added = FALSE;
		context->icon.uFlags = NIF_ICON | NIF_TIP | NIF_INFO;
		if (!context->shell_notify(NIM_ADD, &context->icon))
			return FB_SERVICE_FAILED;
		context->added = TRUE;
	}
	return FB_SERVICE_OK;
}

int fb_hNotifyPoll(void *opaque)
{
	FB_NOTIFY_WIN32 *context = opaque;
	MSG message;
	unsigned int count = 0;
	if (!on_owner(context))
		return FB_SERVICE_WRONG_THREAD;
	/* Never remove WM_QUIT or messages for the application's own windows. */
	while (count++ < 64u && PeekMessageW(&message, context->icon.hWnd,
		0, 0, PM_NOREMOVE)) {
		/* WM_QUIT bypasses even an HWND filter. Leave it for the owner loop. */
		if (message.message == WM_QUIT) break;
		if (!PeekMessageW(&message, context->icon.hWnd, 0, 0, PM_REMOVE)) break;
		TranslateMessage(&message);
		DispatchMessageW(&message);
	}
	return FB_SERVICE_OK;
}

int fb_hNotifyClose(void *opaque)
{
	FB_NOTIFY_WIN32 *context = opaque;
	if (!on_owner(context))
		return FB_SERVICE_WRONG_THREAD;
	if (context->added) {
		context->shell_notify(NIM_DELETE, &context->icon);
		context->added = FALSE;
	}
	return FB_SERVICE_OK;
}

int fb_hNotifyDestroy(void *opaque)
{
	FB_NOTIFY_WIN32 *context = opaque;
	int result = fb_hNotifyClose(context);
	if (result != FB_SERVICE_OK)
		return result;
	if (!DestroyWindow(context->icon.hWnd))
		return FB_SERVICE_FAILED;
	FreeLibrary(context->library);
	free(context);
	return FB_SERVICE_OK;
}

int fb_hNotifyPermission(void *opaque, int request)
{
	(void)request;
	return on_owner(opaque) ? FB_SERVICE_OK : FB_SERVICE_WRONG_THREAD;
}
/* end of sys_hnotify.c */
