/*
 * FreeBASIC graphics tests: win32-drop-source.c
 * Purpose: Deliver a shell-format Unicode file drop to the real window procedure.
 * Responsibilities: Correct HDROP layout and transferred memory ownership.
 * This fixture does not emulate event queue insertion or open dropped files.
 */
#include <windows.h>
#include <shellapi.h>
#include <string.h>

int fb_test_drop(void *window)
{
	/* DROPFILES is a 20-byte header: byte offset, POINT, client flag, wide flag.
	 * The following UTF-16 names have a NUL each and one final list terminator.
	 */
	struct DROP_HEADER { DWORD offset; POINT point; BOOL nonclient; BOOL wide; };
	static const wchar_t names[] = L"C:\\drop\\a b.txt\0C:\\drop\\snow-\u2603-\U0001f30d.txt\0";
	HGLOBAL block = GlobalAlloc(GHND, sizeof(struct DROP_HEADER) + sizeof(names));
	struct DROP_HEADER *header;
	if (block == NULL) return 0;
	header = GlobalLock(block);
	if (header == NULL) { GlobalFree(block); return 0; }
	header->offset = sizeof(*header);
	header->wide = TRUE;
	memcpy((char *)header + sizeof(*header), names, sizeof(names));
	GlobalUnlock(block);
	/* WM_DROPFILES transfers the block even when reception is disabled. */
	SendMessageW(window, WM_DROPFILES, (WPARAM)block, 0);
	return 1;
}
/* end of win32-drop-source.c */
