/*
    Project: FreeBASIC gfxlib3
    --------------------------

    File: gfx3_line_input.c

    Purpose:

        Connect FreeBASIC's interactive string-input operations to the
        gfxlib3 graphical console and keyboard hooks.

    Responsibilities:

        - implement the byte reader used by INPUT$ on standard input
        - route narrow and wide LINE INPUT through the runtime line editor
        - preserve gfxlib2 prompt, question-mark, and newline behavior
        - install the corresponding FreeBASIC runtime hooks

    This file intentionally does NOT contain:

        - native keyboard messages or scan-code translation
        - the full-screen graphical console renderer
        - file input for handles other than standard input
*/

#include "gfx3_console.h"
#include "gfx3_line_input.h"

static const char default_question[] = "? ";

/* ------------------------------------------------------------------------- */
/* INPUT$ byte reader                                                        */
/* ------------------------------------------------------------------------- */

static void line_input_move_back(void)
{
	int columns;
	int rows;
	int column;
	int row;

	fb_GfxGetSize(&columns, &rows);
	fb_GfxGetXY(&column, &row);
	if ((columns <= 0) || (rows <= 0))
		return;
	if (column > 1) {
		column--;
	} else if (row > 1) {
		column = columns;
		row--;
	} else {
		column = 1;
		row = 1;
	}
	fb_GfxLocate(row, column, -1);
}

char *fb_GfxReadStr(char *buffer, ssize_t maximum_length)
{
	static const char cursor_normal[] = { (char)219, '\0' };
	static const char cursor_backspace[] = { (char)219, ' ', '\0' };
	static const char space[] = { ' ', '\0' };
	char character[] = { '\0', '\0' };
	const char *cursor = cursor_normal;
	ssize_t length = 0;
	int key;

	if ((buffer == NULL) || (maximum_length <= 0))
		return NULL;
	FB_GRAPHICS_LOCK();
	do {
		fb_GfxPrintBufferEx(cursor, strlen(cursor), 0);
		if (cursor == cursor_backspace) {
			line_input_move_back();
			cursor = cursor_normal;
		}
		line_input_move_back();

		key = fb_Getkey();
		if (key >= 0x100)
			continue;
		if (key == 8) {
			if (length > 0) {
				cursor = cursor_backspace;
				line_input_move_back();
				length--;
			}
			continue;
		}
		if ((key == 7) || (length >= maximum_length - 1))
			continue;
		if (key == 13) {
			fb_GfxPrintBufferEx(space, 1u, 0);
			line_input_move_back();
		}
		buffer[length++] = (char)key;
		character[0] = (char)key;
		fb_GfxPrintBufferEx(character, 1u, 0);
	} while (key != 13);
	buffer[length] = '\0';
	FB_GRAPHICS_UNLOCK();
	return buffer;
}

/* ------------------------------------------------------------------------- */
/* LINE INPUT runtime hooks                                                  */
/* ------------------------------------------------------------------------- */

int fb_GfxLineInput(FBSTRING *text, void *destination,
	ssize_t destination_length, int fill_remaining, int add_question,
	int add_newline)
{
	FBSTRING *result;

	FB_LOCK();
	fb_PrintBufferEx(NULL, 0, FB_PRINT_FORCE_ADJUST);
	if (text != NULL) {
		if (text->data != NULL)
			fb_PrintString(0, text, 0);
		if (add_question != FB_FALSE)
			fb_PrintFixString(0, default_question, 0);
	}
	FB_UNLOCK();

	result = fb_ConReadLine(TRUE);
	if (add_newline)
		fb_PrintVoid(0, FB_PRINT_NEWLINE);
	if (result == NULL)
		return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
	fb_StrAssign(destination, destination_length, result, -1,
		fill_remaining);
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

int fb_GfxLineInputWstr(const FB_WCHAR *text, FB_WCHAR *destination,
	ssize_t maximum_characters, int add_question, int add_newline)
{
	FBSTRING *result;

	FB_LOCK();
	fb_PrintBufferEx(NULL, 0, FB_PRINT_FORCE_ADJUST);
	if (text != NULL) {
		fb_PrintWstr(0, text, 0);
		if (add_question != FB_FALSE)
			fb_PrintFixString(0, default_question, 0);
	}
	FB_UNLOCK();

	result = fb_ConReadLine(TRUE);
	if (add_newline)
		fb_PrintVoid(0, FB_PRINT_NEWLINE);
	if (result == NULL)
		return fb_ErrorSetNum(FB_RTERROR_OUTOFMEM);
	fb_WstrAssignFromA(destination, maximum_characters, result, -1);
	return fb_ErrorSetNum(FB_RTERROR_OK);
}

/* ------------------------------------------------------------------------- */
/* Hook ownership                                                            */
/* ------------------------------------------------------------------------- */

void fb_gfx3_line_input_install_hooks_locked(void)
{
	__fb_ctx.hooks.readstrproc = fb_GfxReadStr;
	__fb_ctx.hooks.lineinputproc = fb_GfxLineInput;
	__fb_ctx.hooks.lineinputwproc = fb_GfxLineInputWstr;
}

/* end of gfx3_line_input.c */
