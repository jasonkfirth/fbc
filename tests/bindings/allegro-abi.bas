'' Project: FreeBASIC native library regression checks
'' File: allegro-abi.bas
'' Purpose: Compare the classic Allegro binding with independent C layouts.
'' Responsibilities: Bitmap prefix, dialog fields and platform-sized values.
'' This file contains no Allegro 5 compatibility layer or renderer simulation.

#lang "fb"
#include once "allegro.bi"

Print "ABI "; SizeOf(RGB_); " "; SizeOf(DIALOG_); " "; OffsetOf(DIALOG_, dp); " "; _
	SizeOf(SAMPLE); " "; OffsetOf(SAMPLE, len); " "; OffsetOf(SAMPLE, data); " "; _
	SizeOf(FONT_); " "; OffsetOf(FONT_, vtable); " "; OffsetOf(BITMAP_, line(0)); " "; _
	OffsetOf(BITMAP_, id); " "; SizeOf(CULong); " "; SizeOf(GFX_MODE_)

'' end of allegro-abi.bas
