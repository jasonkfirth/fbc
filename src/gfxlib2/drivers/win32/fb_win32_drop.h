/*
 * FreeBASIC graphics: fb_win32_drop.h
 * Purpose: Share the Windows file-drop decoder between gfxlib2 and gfxlib3.
 * Responsibilities: Bounded UTF-16 conversion, atomic delivery and HDROP release.
 * This file does not create windows, open paths or retain shell-owned memory.
 */
#ifndef FB_WIN32_DROP_H
#define FB_WIN32_DROP_H
#include "../../../rtlib/fb_drop.h"
#include <shellapi.h>

typedef int (*FB_WIN32_DROP_SUBMIT)(void *, int, const char *, size_t, int);

static int fb_hWin32DropReceive(HDROP drop, FB_WIN32_DROP_SUBMIT submit, void *user)
{
	typedef UINT (WINAPI *QUERY_FILES)(HDROP, UINT, LPWSTR, UINT);
	typedef void (WINAPI *FINISH_DROP)(HDROP);
	QUERY_FILES query = NULL;
	FINISH_DROP finish = NULL;
	HMODULE library = NULL;
	FARPROC address;
	wchar_t *wide = NULL;
	char *data = NULL;
	wchar_t path[MAX_PATH];
	UINT index, count, length;
	size_t used = 0;
	int result = FB_DROP_INVALID;
	length = GetSystemDirectoryW(path, MAX_PATH);
	if (length == 0 || length >= MAX_PATH - 13u) goto done;
	memcpy(path + length, L"\\shell32.dll", sizeof(L"\\shell32.dll"));
	library = LoadLibraryW(path);
	if (library == NULL) goto done;
	address = GetProcAddress(library, "DragFinish");
	memcpy(&finish, &address, sizeof(finish));
	address = GetProcAddress(library, "DragQueryFileW");
	memcpy(&query, &address, sizeof(query));
	if (query == NULL || finish == NULL) goto done;
	count = query(drop, 0xffffffffu, NULL, 0);
	if (count == 0 || count > FB_DROP_MAX_ITEMS) goto done;
	data = malloc(FB_DROP_MAX_BYTES);
	/* A Win32 extended path is at most 32,767 UTF-16 code units. */
	wide = malloc(32768u * sizeof(*wide));
	if (data == NULL || wide == NULL) { result = FB_DROP_FAILED; goto done; }
	for (index = 0; index < count; ++index) {
		int bytes;
		length = query(drop, index, NULL, 0);
		if (length == 0 || length >= 32768u ||
		    query(drop, index, wide, length + 1u) != length)
			goto done;
		bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide,
			(int)length, data + used, (int)(FB_DROP_MAX_BYTES - used), NULL, NULL);
		if (bytes <= 0 || (size_t)bytes >= FB_DROP_MAX_BYTES - used) goto done;
		used += (size_t)bytes;
		data[used++] = 0;
	}
	result = submit(user, FB_DROP_FILES, data, used, (int)count);
done:
	free(wide);
	free(data);
	if (finish != NULL) finish(drop);
	else GlobalFree((HGLOBAL)drop); /* HDROP is a transferred global-memory block. */
	if (library != NULL) FreeLibrary(library);
	return result;
}
#endif
/* end of fb_win32_drop.h */
