'' Project: FreeBASIC SDL3 bindings
'' File: SDL_textengine.bi
'' Purpose: Declare the SDL3_ttf-3.2.2 C interface for FreeBASIC.
'' Responsibilities: Preserve public types, constants, and calling conventions.
'' This file intentionally does NOT contain: the upstream library implementation.
''
'' Translated from the upstream headers; this is an altered source version.
'' Regenerate with build_scripts/generate-sdl3-bindings.py.
''
'' Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>
''
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
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

#inclib "SDL3_ttf"

#include once "SDL.bi"
#include once "SDL_ttf.bi"


extern "C"



#define SDL_TTF_TEXTENGINE_H_

type TTF_DrawCommand as long
enum
	TTF_DRAW_COMMAND_NOOP
	TTF_DRAW_COMMAND_FILL
	TTF_DRAW_COMMAND_COPY
end enum

type TTF_FillOperation
	cmd as TTF_DrawCommand
	rect as SDL_Rect
end type

type TTF_CopyOperation
	cmd as TTF_DrawCommand
	text_offset as long
	glyph_font as TTF_Font ptr
	glyph_index as Uint32
	src as SDL_Rect
	dst as SDL_Rect
	reserved as any ptr
end type

union TTF_DrawOperation
	cmd as TTF_DrawCommand
	fill as TTF_FillOperation
	copy as TTF_CopyOperation
end union

type TTF_TextData_
	font as TTF_Font ptr
	color as SDL_FColor
	needs_layout_update as boolean
	layout as TTF_TextLayout ptr
	x as long
	y as long
	w as long
	h as long
	num_ops as long
	ops as TTF_DrawOperation ptr
	num_clusters as long
	clusters as TTF_SubString ptr
	props as SDL_PropertiesID
	needs_engine_update as boolean
	engine as TTF_TextEngine ptr
	engine_text as any ptr
end type

type TTF_TextEngine_
	version as Uint32
	userdata as any ptr
	CreateText as function(byval userdata as any ptr, byval text as TTF_Text ptr) as boolean
	DestroyText as sub(byval userdata as any ptr, byval text as TTF_Text ptr)
end type

end extern

'' end of SDL_textengine.bi
