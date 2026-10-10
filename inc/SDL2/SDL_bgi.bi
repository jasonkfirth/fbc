'' Project: FreeBASIC SDL addon bindings
'' File: SDL_bgi.bi
'' Purpose: Declare the pinned SDL2_bgi C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' Copyright (c) 2014-forever Guido Gonzato, PhD
''
'' This software is provided 'as-is', without any express or implied
'' warranty. In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely, subject to the following restrictions:
''
'' 1. The origin of this software must not be misrepresented; you must not
''    claim that you wrote the original software. If you use this software
''    in a product, an acknowledgment in the product documentation would be
''    appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
''    misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#pragma once

#inclib "SDL2_bgi"
#include once "SDL.bi"


#include once "crt/stdint.bi"
#include once "crt/stdlib.bi"
#include once "crt/string.bi"

namespace bgi
extern "C"

#define _SDL_BGI_H
#define __GRAPHICS_H
#define SDL_BGI_VERSION "3.0.4"

enum
	NOPE
	YEAH
end enum

const BGI_WINTITLE_LEN = 512
const NUM_BGI_WIN = 16
const ALL_WINDOWS = -1

extern bgi_window as SDL_Window ptr
extern bgi_renderer as SDL_Renderer ptr
extern bgi_texture as SDL_Texture ptr
const VPAGES = 4

enum
	DEFAULT_FONT
	TRIPLEX_FONT
	SMALL_FONT
	SANS_SERIF_FONT
	GOTHIC_FONT
	SCRIPT_FONT
	SIMPLEX_FONT
	TRIPLEX_SCR_FONT
	COMPLEX_FONT
	EUROPEAN_FONT
	BOLD_FONT
	LAST_SPEC_FONT
end enum

enum
	HORIZ_DIR
	VERT_DIR
end enum

const USER_CHAR_SIZE = 0

enum
	LEFT_TEXT
	CENTER_TEXT
	RIGHT_TEXT
	BOTTOM_TEXT = 0
	TOP_TEXT = 2
end enum

enum
	BLACK = 0
	EGA_BLACK = 0
	BLUE = 1
	EGA_BLUE = 1
	GREEN = 2
	CGA_GREEN = 2
	EGA_GREEN = 2
	CYAN = 3
	CGA_CYAN = 3
	EGA_CYAN = 3
	RED = 4
	CGA_RED = 4
	EGA_RED = 4
	MAGENTA = 5
	CGA_MAGENTA = 5
	EGA_MAGENTA = 5
	BROWN = 6
	CGA_BROWN = 6
	EGA_BROWN = 6
	LIGHTGRAY = 7
	CGA_LIGHTGRAY = 7
	EGA_LIGHTGRAY = 7
	DARKGRAY = 8
	EGA_DARKGRAY = 8
	LIGHTBLUE = 9
	EGA_LIGHTBLUE = 9
	LIGHTGREEN = 10
	CGA_LIGHTGREEN = 10
	EGA_LIGHTGREEN = 10
	LIGHTCYAN = 11
	CGA_LIGHTCYAN = 11
	EGA_LIGHTCYAN = 11
	LIGHTRED = 12
	CGA_LIGHTRED = 12
	EGA_LIGHTRED = 12
	LIGHTMAGENTA = 13
	CGA_LIGHTMAGENTA = 13
	EGA_LIGHTMAGENTA = 13
	YELLOW = 14
	CGA_YELLOW = 14
	EGA_YELLOW = 14
	WHITE = 15
	CGA_WHITE = 15
	EGA_WHITE = 15
	MAXCOLORS = 15
end enum

enum
	ARGB_FG_COL = 16
	ARGB_BG_COL = 17
	ARGB_FILL_COL = 18
	ARGB_TMP_COL = 19
	TMP_COLORS = 4
end enum

enum
	NORM_WIDTH = 1
	THICK_WIDTH = 3
end enum

enum
	SOLID_LINE
	DOTTED_LINE
	CENTER_LINE
	DASHED_LINE
	USERBIT_LINE
end enum

enum
	COPY_PUT
	XOR_PUT
	OR_PUT
	AND_PUT
	NOT_PUT
end enum

enum
	EMPTY_FILL
	SOLID_FILL
	LINE_FILL
	LTSLASH_FILL
	SLASH_FILL
	BKSLASH_FILL
	LTBKSLASH_FILL
	HATCH_FILL
	XHATCH_FILL
	INTERLEAVE_FILL
	WIDE_DOT_FILL
	CLOSE_DOT_FILL
	USER_FILL
end enum

const WM_MOUSEMOVE = SDL_MOUSEMOTION
const WM_LBUTTONDOWN = SDL_BUTTON_LEFT
const WM_LBUTTONUP = (SDL_MOUSEBUTTONUP + SDL_BUTTON_LEFT)
const WM_LBUTTONDBLCLK = ((SDL_MOUSEBUTTONDOWN + SDL_BUTTON_LEFT) + 2)
const WM_MBUTTONDOWN = SDL_BUTTON_MIDDLE
const WM_MBUTTONUP = (SDL_MOUSEBUTTONUP + (10 * SDL_BUTTON_MIDDLE))
const WM_MBUTTONDBLCLK = ((SDL_MOUSEBUTTONDOWN + (10 * SDL_BUTTON_MIDDLE)) + 2)
const WM_RBUTTONDOWN = SDL_BUTTON_RIGHT
const WM_RBUTTONUP = (SDL_MOUSEBUTTONUP + (20 * SDL_BUTTON_RIGHT))
const WM_RBUTTONDBLCLK = ((SDL_MOUSEBUTTONDOWN + (20 * SDL_BUTTON_RIGHT)) + 2)
const WM_WHEEL = SDL_MOUSEWHEEL
const WM_WHEELUP = (SDL_BUTTON_RIGHT + 1)
const WM_WHEELDOWN = (SDL_BUTTON_RIGHT + 2)
#define KEY_HOME SDLK_HOME
#define KEY_LEFT SDLK_LEFT
#define KEY_UP SDLK_UP
#define KEY_RIGHT SDLK_RIGHT
#define KEY_DOWN SDLK_DOWN
#define KEY_PGUP SDLK_PAGEUP
#define KEY_PGDN SDLK_PAGEDOWN
#define KEY_END SDLK_END
#define KEY_INSERT SDLK_INSERT
#define KEY_DELETE SDLK_DELETE
#define KEY_F1 SDLK_F1
#define KEY_F2 SDLK_F2
#define KEY_F3 SDLK_F3
#define KEY_F4 SDLK_F4
#define KEY_F5 SDLK_F5
#define KEY_F6 SDLK_F6
#define KEY_F7 SDLK_F7
#define KEY_F8 SDLK_F8
#define KEY_F9 SDLK_F9
#define KEY_F10 SDLK_F10
#define KEY_F11 SDLK_F11
#define KEY_F12 SDLK_F12
#define KEY_CAPSLOCK SDLK_CAPSLOCK
#define KEY_LEFT_CTRL SDLK_LCTRL
#define KEY_RIGHT_CTRL SDLK_RCTRL
#define KEY_LEFT_SHIFT SDLK_LSHIFT
#define KEY_RIGHT_SHIFT SDLK_RSHIFT
#define KEY_LEFT_ALT SDLK_LALT
#define KEY_RIGHT_ALT SDLK_RALT
#define KEY_ALT_GR SDLK_MODE
#define KEY_LGUI SDLK_LGUI
#define KEY_RGUI SDLK_RGUI
#define KEY_MENU SDLK_MENU
#define KEY_TAB SDLK_TAB
#define KEY_BS SDLK_BACKSPACE
#define KEY_RET SDLK_RETURN
#define KEY_PAUSE SDLK_PAUSE
#define KEY_SCR_LOCK SDLK_SCROLLOCK
#define KEY_ESC SDLK_ESCAPE
#define QUIT SDL_QUIT

enum
	DETECT = -1
	SDL = 0
	SDL_320x200 = 1
	SDL_CGALO = 1
	CGA = 1
	CGAC0 = 1
	CGAC1 = 1
	CGAC2 = 1
	CGAC3 = 1
	MCGAC0 = 1
	MCGAC1 = 1
	MCGAC2 = 1
	MCGAC3 = 1
	ATT400C0 = 1
	ATT400C1 = 1
	ATT400C2 = 1
	ATT400C3 = 1
	SDL_640x200 = 2
	SDL_CGAHI = 2
	CGAHI = 2
	MCGAMED = 2
	EGALO = 2
	EGA64LO = 2
	SDL_640x350 = 3
	SDL_EGA = 3
	EGA = 3
	EGAHI = 3
	EGA64HI = 3
	EGAMONOHI = 3
	SDL_640x480 = 4
	SDL_VGA = 4
	VGA = 4
	MCGAHI = 4
	VGAHI = 4
	IBM8514LO = 4
	SDL_720x348 = 5
	SDL_HERC = 5
	SDL_720x350 = 6
	SDL_PC3270 = 6
	HERCMONOHI = 6
	SDL_800x600 = 7
	SDL_SVGALO = 7
	SVGA = 7
	SDL_1024x768 = 8
	SDL_SVGAMED1 = 8
	SDL_1152x900 = 9
	SDL_SVGAMED2 = 9
	SDL_1280x1024 = 10
	SDL_SVGAHI = 10
	SDL_1366x768 = 11
	SDL_HD = 11
	SDL_1920x1080 = 12
	SDL_FHD = 12
	SDL_USER = 13
	SDL_FULLSCREEN = 14
end enum

type graphics_errors as long
enum
	grOk = 0
	grNoInitGraph = -1
	grNotDetected = -2
	grFileNotFound = -3
	grInvalidDriver = -4
	grNoLoadMem = -5
	grNoScanMem = -6
	grNoFloodMem = -7
	grFontNotFound = -8
	grNoFontMem = -9
	grInvalidMode = -10
	grError = -11
	grIOerror = -12
	grInvalidFont = -13
	grInvalidFontNum = -14
	grInvalidVersion = -18
end enum

const X11_CGALO = SDL_CGALO
const X11_CGAHI = SDL_CGAHI
const X11_EGA = SDL_EGA
const X11 = SDL
const X11_VGA = SDL_VGA
const X11_640x480 = SDL_640x480
const X11_HERC = SDL_HERC
const X11_PC3270 = SDL_PC3270
const X11_SVGALO = SDL_SVGALO
const X11_800x600 = SDL_800x600
const X11_SVGAMED1 = SDL_SVGAMED1
const X11_1024x768 = SDL_1024x768
const X11_SVGAMED2 = SDL_SVGAMED2
const X11_1152x900 = SDL_1152x900
const X11_SVGAHI = SDL_SVGAHI
const X11_1280x1024 = SDL_1280x1024
#define X11_WXGA SDL_WXGA
const X11_1366x768 = SDL_1366x768
const X11_USER = SDL_USER
const X11_FULLSCREEN = SDL_FULLSCREEN

type arccoordstype
	x as long
	y as long
	xstart as long
	ystart as long
	xend as long
	yend as long
end type

type date
	da_year as long
	da_day as long
	da_mon as long
end type

type fillsettingstype
	pattern as long
	color as long
end type

type linesettingstype
	linestyle as long
	upattern as ulong
	thickness as long
end type

type palettetype
	size as ubyte
	colors(0 to (MAXCOLORS + 1) - 1) as Uint32
end type

type rgbpalettetype
	size as Uint32
	colors as Uint32 ptr
end type

type textsettingstype
	font as long
	direction as long
	charsize as long
	horiz as long
	vert as long
end type

type viewporttype
	left as long
	top as long
	right as long
	bottom as long
	clip as long
end type

declare sub arc(byval as long, byval as long, byval as long, byval as long, byval as long)
declare sub bar3d(byval as long, byval as long, byval as long, byval as long, byval as long, byval as long)
declare sub bar(byval as long, byval as long, byval as long, byval as long)
declare sub circle(byval as long, byval as long, byval as long)
declare sub cleardevice()
declare sub clearviewport()
declare sub closegraph()
declare sub delay(byval as long)
declare sub detectgraph(byval as long ptr, byval as long ptr)
declare sub drawpoly(byval as long, byval as long ptr)
declare sub ellipse(byval as long, byval as long, byval as long, byval as long, byval as long, byval as long)
declare sub fillellipse(byval as long, byval as long, byval as long, byval as long)
declare sub fillpoly(byval as long, byval as long ptr)
declare sub floodfill(byval as long, byval as long, byval as long)
declare function getactivepage() as long
declare sub getarccoords(byval as arccoordstype ptr)
declare sub getaspectratio(byval as long ptr, byval as long ptr)
declare function getbkcolor() as long
declare function bgi_getch() as long
declare function getch alias "bgi_getch"() as long
declare function getcolor() as long
declare function getdefaultpalette() as palettetype ptr
declare function getdrivername() as zstring ptr
declare sub getfillpattern(byval as zstring ptr)
declare sub getfillsettings(byval as fillsettingstype ptr)
declare function getgraphmode() as long
declare sub getimage(byval as long, byval as long, byval as long, byval as long, byval as any ptr)
declare sub getlinesettings(byval as linesettingstype ptr)
declare function getmaxcolor() as long
declare function getmaxmode() as long
declare function getmaxx() as long
declare function getmaxy() as long
declare function getmodename(byval as long) as zstring ptr
declare sub getmoderange(byval as long, byval as long ptr, byval as long ptr)
declare sub getpalette(byval as palettetype ptr)
declare function getpalettesize() as long
declare function getpixel(byval as long, byval as long) as ulong
declare sub gettextsettings(byval as textsettingstype ptr)
declare sub getviewsettings(byval as viewporttype ptr)
declare function getvisualpage() as long
declare function getx() as long
declare function gety() as long
declare sub graphdefaults()
declare function grapherrormsg(byval as long) as zstring ptr
declare function graphresult() as long
declare function imagesize(byval as long, byval as long, byval as long, byval as long) as ulong
declare sub initgraph(byval as long ptr, byval as long ptr, byval as zstring ptr)
declare function installuserdriver(byval as zstring ptr, byval as function() as long) as long
declare function installuserfont(byval as zstring ptr) as long
declare function k_bhit() as long
declare function kbhit alias "k_bhit"() as long
declare function lastkey() as long
declare sub line(byval as long, byval as long, byval as long, byval as long)
declare sub linerel(byval as long, byval as long)
declare sub lineto(byval as long, byval as long)
declare sub moverel(byval as long, byval as long)
declare sub moveto(byval as long, byval as long)
declare sub outtext(byval as zstring ptr)
declare sub outtextxy(byval as long, byval as long, byval as zstring ptr)
declare sub pieslice(byval as long, byval as long, byval as long, byval as long, byval as long)
declare sub putimage(byval as long, byval as long, byval as any ptr, byval as long)
declare sub putpixel(byval as long, byval as long, byval as long)
#define bgi_random(range) (rand() mod (range))
declare sub rectangle(byval as long, byval as long, byval as long, byval as long)
declare function registerbgidriver(byval as sub()) as long
declare function registerbgifont(byval as sub()) as long
declare sub restorecrtmode()
declare sub sector(byval as long, byval as long, byval as long, byval as long, byval as long, byval as long)
declare sub setactivepage(byval as long)
declare sub setallpalette(byval as palettetype ptr)
declare sub setaspectratio(byval as long, byval as long)
declare sub setbkcolor(byval as long)
declare sub setcolor(byval as long)
declare sub setfillpattern(byval as zstring ptr, byval as long)
declare sub setfillstyle(byval as long, byval as long)
declare function setgraphbufsize(byval as ulong) as ulong
declare sub setgraphmode(byval as long)
declare sub setlinestyle(byval as long, byval as ulong, byval as long)
declare sub setpalette(byval as long, byval as long)
declare sub settextjustify(byval as long, byval as long)
declare sub settextstyle(byval as long, byval as long, byval as long)
declare sub setusercharsize(byval as long, byval as long, byval as long, byval as long)
declare sub setviewport(byval as long, byval as long, byval as long, byval as long, byval as long)
declare sub setvisualpage(byval as long)
declare sub setwritemode(byval as long)
declare function textheight(byval as zstring ptr) as long
declare function textwidth(byval as zstring ptr) as long
declare function ALPHA_VALUE(byval as long) as long
declare function BLUE_VALUE(byval as long) as long
declare sub closewindow(byval as long)
declare function COLOR(byval as long, byval as long, byval as long) as long
declare function COLOR32(byval as Uint32) as long
declare sub clearmouseclick(byval as long)
declare function colorname(byval as long) as zstring ptr
#define colorRGB(r, g, b) (((&hff000000 or ((r) shl 16)) or ((g) shl 8)) or (b))
declare sub copysurface(byval as SDL_Surface ptr, byval as long, byval as long, byval as long, byval as long)
declare function doubleclick() as long
declare function edelay(byval as long) as long
declare function event() as long
declare function eventtype() as long
declare sub fputpixel(byval as long, byval as long)
declare sub getbuffer(byval as Uint32 ptr)
declare function getclick() as long
declare function getcurrentwindow() as long
declare sub getleftclick()
declare function getevent() as long
declare sub getlinebuffer(byval as long, byval as Uint32 ptr)
declare function getmaxheight() as long
declare function getmaxwidth() as long
declare sub getmiddleclick()
declare sub getmouseclick(byval as long, byval as long ptr, byval as long ptr)
declare sub getrgbpalette(byval as rgbpalettetype ptr, byval as long)
declare function getrgbpalettesize() as long
declare sub getrightclick()
declare sub getscreensize(byval as long ptr, byval as long ptr)
declare function getwindowwidth alias "getmaxx"() as long
declare function getwindowheight alias "getmaxy"() as long
declare function GREEN_VALUE(byval as long) as long
declare sub initpalette()
declare function _initwin_1(byval as long, byval as long) as long
declare function _initwin_2(byval as long, byval as long, byval as zstring ptr) as long
declare function IS_BGI_COLOR(byval color as long) as long
declare function ismouseclick(byval as long) as long
declare function IS_RGB_COLOR(byval color as long) as long
declare function kdelay(byval as long) as long
declare function mouseclick() as long
declare function mousex() as long
declare function mousey() as long
declare sub putbuffer(byval as Uint32 ptr)
declare sub putlinebuffer(byval as long, byval as Uint32 ptr)
declare sub _putpixel alias "fputpixel"(byval as long, byval as long)
declare function RED_VALUE(byval as long) as long
declare function RGBPALETTE(byval as long) as long
declare sub readimagefile(byval as zstring ptr, byval as long, byval as long, byval as long, byval as long)
declare sub refresh()
declare sub resetwinoptions(byval as long, byval as zstring ptr, byval as long, byval as long)
declare function resizepalette(byval as Uint32) as long
declare sub sdlbgiauto()
declare sub sdlbgifast()
declare sub sdlbgislow()
declare sub setallrgbpalette(byval as rgbpalettetype ptr)
declare sub setalpha(byval as long, byval as Uint8)
declare sub setbkrgbcolor(byval as long)
declare sub setblendmode(byval as long)
declare sub setcurrentwindow(byval as long)
declare sub setrgbcolor(byval as long)
declare sub setrgbpalette(byval as long, byval as long, byval as long, byval as long)
declare sub setwinoptions(byval as zstring ptr, byval as long, byval as long, byval as Uint32)
declare sub setwintitle(byval as long, byval as zstring ptr)
declare sub showinfobox(byval as const zstring ptr)
declare sub showerrorbox(byval as const zstring ptr)
declare sub swapbuffers()
declare sub writeimagefile(byval as zstring ptr, byval as long, byval as long, byval as long, byval as long)
declare function xkb_hit() as long
declare function xkbhit alias "xkb_hit"() as long

end extern

end namespace

namespace bgi
private function initwindow cdecl(byval window_width as long, byval window_height as long, _
	byval title as zstring ptr = 0) as long
	if title = 0 then return _initwin_1(window_width, window_height)
	return _initwin_2(window_width, window_height, title)
end function
end namespace

'' End of SDL_bgi.bi
