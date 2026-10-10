'' Project: FreeBASIC SDL addon bindings
'' File: SDL_rtf.bi
'' Purpose: Declare the pinned SDL_rtf C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' SDL_rtf:  A companion library to SDL for displaying RTF format text
''   Copyright (C) 2003-2012 Sam Lantinga <slouken@libsdl.org>
''
''   This software is provided 'as-is', without any express or implied
''   warranty.  In no event will the authors be held liable for any damages
''   arising from the use of this software.
''
''   Permission is granted to anyone to use this software for any purpose,
''   including commercial applications, and to alter it and redistribute it
''   freely, subject to the following restrictions:
''
''   1. The origin of this software must not be misrepresented; you must not
''      claim that you wrote the original software. If you use this software
''      in a product, an acknowledgment in the product documentation would be
''      appreciated but is not required.
''   2. Altered source versions must be plainly marked as such, and must not be
''      misrepresented as being the original software.
''   3. This notice may not be removed or altered from any source distribution.

#pragma once

#inclib "SDL_rtf"
#include once "SDL.bi"


extern "C"

#define SDL_RTF_H_
const SDL_RTF_MAJOR_VERSION = 0
const SDL_RTF_MINOR_VERSION = 1
const SDL_RTF_PATCHLEVEL = 1
#macro SDL_RTF_VERSION(X)
	scope
		(X)->major = SDL_RTF_MAJOR_VERSION
		(X)->minor = SDL_RTF_MINOR_VERSION
		(X)->patch = SDL_RTF_PATCHLEVEL
	end scope
#endmacro
const RTF_MAJOR_VERSION = SDL_RTF_MAJOR_VERSION
const RTF_MINOR_VERSION = SDL_RTF_MINOR_VERSION
const RTF_PATCHLEVEL = SDL_RTF_PATCHLEVEL
#define RTF_VERSION(X) SDL_RTF_VERSION(X)
declare function RTF_Linked_Version() as const SDL_version ptr
type RTF_Context as _RTF_Context

type RTF_FontFamily as long
enum
	RTF_FontDefault
	RTF_FontRoman
	RTF_FontSwiss
	RTF_FontModern
	RTF_FontScript
	RTF_FontDecor
	RTF_FontTech
	RTF_FontBidi
end enum

type RTF_FontStyle as long
enum
	RTF_FontNormal = &h00
	RTF_FontBold = &h01
	RTF_FontItalic = &h02
	RTF_FontUnderline = &h04
end enum

const RTF_FONT_ENGINE_VERSION = 1

type _RTF_FontEngine
	version as long
	CreateFont as function(byval name as const zstring ptr, byval family as RTF_FontFamily, byval charset as long, byval size as long, byval style as long) as any ptr
	GetLineSpacing as function(byval font as any ptr) as long
	GetCharacterOffsets as function(byval font as any ptr, byval text as const zstring ptr, byval byteOffsets as long ptr, byval pixelOffsets as long ptr, byval maxOffsets as long) as long
	RenderText as function(byval font as any ptr, byval text as const zstring ptr, byval fg as SDL_Color) as SDL_Surface ptr
	FreeFont as sub(byval font as any ptr)
end type

type RTF_FontEngine as _RTF_FontEngine
declare function RTF_CreateContext(byval fontEngine as RTF_FontEngine ptr) as RTF_Context ptr
declare function RTF_Load(byval ctx as RTF_Context ptr, byval file as const zstring ptr) as long
declare function RTF_Load_RW(byval ctx as RTF_Context ptr, byval src as SDL_RWops ptr, byval freesrc as long) as long
declare function RTF_GetTitle(byval ctx as RTF_Context ptr) as const zstring ptr
declare function RTF_GetSubject(byval ctx as RTF_Context ptr) as const zstring ptr
declare function RTF_GetAuthor(byval ctx as RTF_Context ptr) as const zstring ptr
declare function RTF_GetHeight(byval ctx as RTF_Context ptr, byval width as long) as long
declare sub RTF_Render(byval ctx as RTF_Context ptr, byval surface as SDL_Surface ptr, byval rect as SDL_Rect ptr, byval yOffset as long)
declare sub RTF_FreeContext(byval ctx as RTF_Context ptr)
#define RTF_SetError SDL_SetError
#define RTF_GetError SDL_GetError

end extern

'' End of SDL_rtf.bi
