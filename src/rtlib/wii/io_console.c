/*
    FreeBASIC runtime Wii console hooks
    -----------------------------------

    File: io_console.c

    Purpose:

        Provide the small console hook surface required by generic rtlib code.

    Responsibilities:

        - keep console coordinate, size, CLS, WIDTH, and LOCATE hooks linkable
        - report no console mouse when graphics mode is not active
        - let SLEEP poll the existing Wii console key stub

    This file intentionally does NOT contain:

        - gfxlib mouse handling
        - keyboard device support
        - a scrollback buffer or terminal emulator
*/

#include "../fb.h"

static int console_col = 1;
static int console_row = 1;
static int console_cols = 80;
static int console_rows = 25;
static unsigned int console_fg = 7;
static unsigned int console_bg = 0;

unsigned int fb_ConsoleColor(unsigned int fc, unsigned int bc, int flags)
{
	static const int ansi_color[8] = {
		30, 34, 32, 36, 31, 35, 33, 37
	};
	unsigned int old_color = console_fg | (console_bg << 16);

	/*
		libogc's console understands the basic ANSI color escape sequence.
		The runtime COLOR statement uses the traditional DOS color order, so
		this small table maps the low three bits into ANSI foreground codes.
		Bright attributes are intentionally not forced here because the Wii
		console is only a simple startup/debug console.
	*/
	if ((flags & FB_COLOR_FG_DEFAULT) == 0)
		console_fg = fc & 0x0f;
	if ((flags & FB_COLOR_BG_DEFAULT) == 0)
		console_bg = bc & 0x0f;

	fb_WiiVideoInit();
	fprintf(stdout, "\x1b[%d;%dm",
		ansi_color[console_fg & 0x07],
		ansi_color[console_bg & 0x07] + 10);
	fflush(stdout);

	return old_color;
}

unsigned int fb_ConsoleGetColorAtt(void)
{
	return console_fg | (console_bg << 4);
}

void fb_ConsoleClear(int mode)
{
	(void)mode;

	fb_WiiVideoInit();
	fputs("\x1b[2J\x1b[H", stdout);
	fflush(stdout);
	console_col = 1;
	console_row = 1;
}

int fb_ConsoleLocate(int row, int col, int cursor)
{
	(void)cursor;

	if (row > 0)
		console_row = row;
	if (col > 0)
		console_col = col;

	fb_WiiVideoInit();
	fprintf(stdout, "\x1b[%d;%dH", console_row, console_col);
	fflush(stdout);
	return 0;
}

int fb_ConsoleGetX(void)
{
	return console_col;
}

int fb_ConsoleGetY(void)
{
	return console_row;
}

FBCALL void fb_ConsoleGetXY(int *col, int *row)
{
	if (col != NULL)
		*col = console_col;
	if (row != NULL)
		*row = console_row;
}

FBCALL void fb_ConsoleGetSize(int *cols, int *rows)
{
	if (cols != NULL)
		*cols = console_cols;
	if (rows != NULL)
		*rows = console_rows;
}

int fb_ConsoleWidth(int cols, int rows)
{
	if (cols > 0)
		console_cols = cols;
	if (rows > 0)
		console_rows = rows;

	return 0;
}

void fb_ConsoleViewUpdate(void)
{
}

int fb_hConsoleInputBufferChanged(void)
{
	return fb_ConsoleKeyHit() != 0;
}

int fb_ConsoleGetMouse(int *x, int *y, int *z, int *buttons, int *clip)
{
	if (x) *x = -1;
	if (y) *y = -1;
	if (z) *z = -1;
	if (buttons) *buttons = -1;
	if (clip) *clip = -1;
	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

int fb_ConsoleSetMouse(int x, int y, int cursor, int clip)
{
	(void)x;
	(void)y;
	(void)cursor;
	(void)clip;
	return fb_ErrorSetNum(FB_RTERROR_ILLEGALFUNCTIONCALL);
}

/* end of io_console.c */
