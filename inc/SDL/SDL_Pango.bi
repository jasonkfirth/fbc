'' Project: FreeBASIC SDL addon bindings
'' File: SDL_Pango.bi
'' Purpose: Declare the pinned SDL_Pango C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' SDL_Pango.h -- A companion library to SDL for working with Pango.
''     Copyright (C) 2004 NAKAMURA Ken'ichi
''
''     This library is free software; you can redistribute it and/or
''     modify it under the terms of the GNU Lesser General Public
''     License as published by the Free Software Foundation; either
''     version 2.1 of the License, or (at your option) any later version.
''
''     This library is distributed in the hope that it will be useful,
''     but WITHOUT ANY WARRANTY; without even the implied warranty of
''     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
''     Lesser General Public License for more details.
''
''     You should have received a copy of the GNU Lesser General Public
''     License along with this library; if not, write to the Free Software
''     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.

#pragma once

#inclib "SDL_Pango"
#include once "SDL.bi"


#include once "pango/pango.bi"
#include once "freetype2/freetype.bi"

extern "C"

#define SDL_PANGO_H
type SDLPango_Context as _contextImpl

type _SDLPango_Matrix
	m(0 to 3, 0 to 3) as Uint8
end type

type SDLPango_Matrix as _SDLPango_Matrix

type SDLPango_Direction as long
enum
	SDLPANGO_DIRECTION_LTR
	SDLPANGO_DIRECTION_RTL
	SDLPANGO_DIRECTION_WEAK_LTR
	SDLPANGO_DIRECTION_WEAK_RTL
	SDLPANGO_DIRECTION_NEUTRAL
end enum

declare function SDLPango_Init() as long
declare function SDLPango_WasInit() as long
declare function SDLPango_CreateContext() as SDLPango_Context ptr
declare sub SDLPango_FreeContext(byval context as SDLPango_Context ptr)
declare sub SDLPango_SetSurfaceCreateArgs(byval context as SDLPango_Context ptr, byval flags as Uint32, byval depth as long, byval Rmask as Uint32, byval Gmask as Uint32, byval Bmask as Uint32, byval Amask as Uint32)
declare function SDLPango_CreateSurfaceDraw(byval context as SDLPango_Context ptr) as SDL_Surface ptr
declare sub SDLPango_Draw(byval context as SDLPango_Context ptr, byval surface as SDL_Surface ptr, byval x as long, byval y as long)
declare sub SDLPango_SetDpi(byval context as SDLPango_Context ptr, byval dpi_x as double, byval dpi_y as double)
declare sub SDLPango_SetMinimumSize(byval context as SDLPango_Context ptr, byval width as long, byval height as long)
declare sub SDLPango_SetDefaultColor(byval context as SDLPango_Context ptr, byval color_matrix as const SDLPango_Matrix ptr)
declare function SDLPango_GetLayoutWidth(byval context as SDLPango_Context ptr) as long
declare function SDLPango_GetLayoutHeight(byval context as SDLPango_Context ptr) as long
declare sub SDLPango_SetMarkup(byval context as SDLPango_Context ptr, byval markup as const zstring ptr, byval length as long)
declare sub SDLPango_SetText(byval context as SDLPango_Context ptr, byval markup as const zstring ptr, byval length as long)
declare sub SDLPango_SetLanguage(byval context as SDLPango_Context ptr, byval language_tag as const zstring ptr)
declare sub SDLPango_SetBaseDirection(byval context as SDLPango_Context ptr, byval direction as SDLPango_Direction)

private function SDLPango_MATRIX_WHITE_BACK cdecl() as const SDLPango_Matrix ptr
	static matrix as SDLPango_Matrix = ({ {255, 255, 255, 255}, {0, 0, 0, 255}, {0, 0, 0, 0}, {0, 0, 0, 0} })
	return @matrix
end function
#define MATRIX_WHITE_BACK SDLPango_MATRIX_WHITE_BACK()
private function SDLPango_MATRIX_BLACK_BACK cdecl() as const SDLPango_Matrix ptr
	static matrix as SDLPango_Matrix = ({ {0, 0, 0, 255}, {255, 255, 255, 255}, {0, 0, 0, 0}, {0, 0, 0, 0} })
	return @matrix
end function
#define MATRIX_BLACK_BACK SDLPango_MATRIX_BLACK_BACK()
private function SDLPango_MATRIX_TRANSPARENT_BACK_BLACK_LETTER cdecl() as const SDLPango_Matrix ptr
	static matrix as SDLPango_Matrix = ({ {0, 0, 0, 0}, {0, 0, 0, 255}, {0, 0, 0, 0}, {0, 0, 0, 0} })
	return @matrix
end function
#define MATRIX_TRANSPARENT_BACK_BLACK_LETTER SDLPango_MATRIX_TRANSPARENT_BACK_BLACK_LETTER()
private function SDLPango_MATRIX_TRANSPARENT_BACK_WHITE_LETTER cdecl() as const SDLPango_Matrix ptr
	static matrix as SDLPango_Matrix = ({ {255, 255, 255, 0}, {255, 255, 255, 255}, {0, 0, 0, 0}, {0, 0, 0, 0} })
	return @matrix
end function
#define MATRIX_TRANSPARENT_BACK_WHITE_LETTER SDLPango_MATRIX_TRANSPARENT_BACK_WHITE_LETTER()
private function SDLPango_MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER cdecl() as const SDLPango_Matrix ptr
	static matrix as SDLPango_Matrix = ({ {255, 255, 255, 0}, {255, 255, 255, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} })
	return @matrix
end function
#define MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER SDLPango_MATRIX_TRANSPARENT_BACK_TRANSPARENT_LETTER()

declare sub SDLPango_CopyFTBitmapToSurface(byval bitmap as const FT_Bitmap ptr, _
	byval surface as SDL_Surface ptr, byval matrix as const SDLPango_Matrix ptr, byval rect as SDL_Rect ptr)
declare function SDLPango_GetPangoFontMap(byval context as SDLPango_Context ptr) as PangoFontMap ptr
declare function SDLPango_GetPangoFontDescription(byval context as SDLPango_Context ptr) as PangoFontDescription ptr
declare function SDLPango_GetPangoLayout(byval context as SDLPango_Context ptr) as PangoLayout ptr

end extern

'' End of SDL_Pango.bi
