'' Project: FreeBASIC SDL addon bindings
'' File: SDL_FontCache.bi
'' Purpose: Declare the pinned SDL2_FontCache C interface.
'' Responsibilities: Preserve public types, callbacks, and header helpers.
'' This file intentionally does NOT contain: the external library implementation.
'' Translated from upstream; this is an altered source version.
''
'' SDL_FontCache v0.10.0: A font cache for SDL and SDL_ttf
'' by Jonathan Dearborn
'' Dedicated to the memory of Florian Hufsky
''
'' License:
''     The short:
''     Use it however you'd like, but keep the copyright and license notice
''     whenever these files or parts of them are distributed in uncompiled form.
''
''     The long:
'' Copyright (c) 2019 Jonathan Dearborn
''
'' Permission is hereby granted, free of charge, to any person obtaining a copy
'' of this software and associated documentation files (the "Software"), to deal
'' in the Software without restriction, including without limitation the rights
'' to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
'' copies of the Software, and to permit persons to whom the Software is
'' furnished to do so, subject to the following conditions:
''
'' The above copyright notice and this permission notice shall be included in
'' all copies or substantial portions of the Software.
''
'' THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
'' IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
'' FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
'' AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
'' LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
'' OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
'' THE SOFTWARE.

#pragma once

#inclib "SDL2_FontCache"
#include once "SDL.bi"
#include once "SDL_ttf.bi"


extern "C"

type FC_Font as FC_Font_

#define _SDL_FONTCACHE_H__
const TTF_STYLE_OUTLINE = 16
#define FC_Rect SDL_Rect
#define FC_Target SDL_Renderer
#define FC_Image SDL_Texture
#define FC_Log SDL_Log

type FC_AlignEnum as long
enum
	FC_ALIGN_LEFT
	FC_ALIGN_CENTER
	FC_ALIGN_RIGHT
end enum

type FC_FilterEnum as long
enum
	FC_FILTER_NEAREST
	FC_FILTER_LINEAR
end enum

type FC_Scale
	x as single
	y as single
end type

type FC_Effect
	alignment as FC_AlignEnum
	scale as FC_Scale
	color as SDL_Color
end type

type FC_GlyphData
	rect as SDL_Rect
	cache_level as long
end type

declare function FC_MakeRect(byval x as single, byval y as single, byval w as single, byval h as single) as SDL_Rect
declare function FC_MakeScale(byval x as single, byval y as single) as FC_Scale
declare function FC_MakeColor(byval r as Uint8, byval g as Uint8, byval b as Uint8, byval a as Uint8) as SDL_Color
declare function FC_MakeEffect(byval alignment as FC_AlignEnum, byval scale as FC_Scale, byval color as SDL_Color) as FC_Effect
declare function FC_MakeGlyphData(byval cache_level as long, byval x as Sint16, byval y as Sint16, byval w as Uint16, byval h as Uint16) as FC_GlyphData
declare function FC_CreateFont() as FC_Font ptr
declare function FC_LoadFont(byval font as FC_Font ptr, byval renderer as SDL_Renderer ptr, byval filename_ttf as const zstring ptr, byval pointSize as Uint32, byval color as SDL_Color, byval style as long) as Uint8
declare function FC_LoadFontFromTTF(byval font as FC_Font ptr, byval renderer as SDL_Renderer ptr, byval ttf as TTF_Font ptr, byval color as SDL_Color) as Uint8
declare function FC_LoadFont_RW(byval font as FC_Font ptr, byval renderer as SDL_Renderer ptr, byval file_rwops_ttf as SDL_RWops ptr, byval own_rwops as Uint8, byval pointSize as Uint32, byval color as SDL_Color, byval style as long) as Uint8
declare sub FC_ResetFontFromRendererReset(byval font as FC_Font ptr, byval renderer as SDL_Renderer ptr, byval evType as Uint32)
declare sub FC_ClearFont(byval font as FC_Font ptr)
declare sub FC_FreeFont(byval font as FC_Font ptr)
declare function FC_GetStringASCII() as zstring ptr
declare function FC_GetStringLatin1() as zstring ptr
declare function FC_GetStringASCII_Latin1() as zstring ptr
declare function FC_GetCodepointFromUTF8(byval c as const zstring ptr ptr, byval advance_pointer as Uint8) as Uint32
declare sub FC_GetUTF8FromCodepoint(byval result as zstring ptr, byval codepoint as Uint32)
declare function U8_alloc(byval size as ulong) as zstring ptr
declare sub U8_free(byval string as zstring ptr)
declare function U8_strdup(byval string as const zstring ptr) as zstring ptr
declare function U8_strlen(byval string as const zstring ptr) as long
declare function U8_charsize(byval character as const zstring ptr) as long
declare function U8_charcpy(byval buffer as zstring ptr, byval source as const zstring ptr, byval buffer_size as long) as long
declare function U8_next(byval string as const zstring ptr) as const zstring ptr
declare function U8_strinsert(byval string as zstring ptr, byval position as long, byval source as const zstring ptr, byval max_bytes as long) as long
declare sub U8_strdel(byval string as zstring ptr, byval position as long)
declare sub FC_SetLoadingString(byval font as FC_Font ptr, byval string as const zstring ptr)
declare function FC_GetBufferSize() as ulong
declare sub FC_SetBufferSize(byval size as ulong)
declare function FC_GetTabWidth() as ulong
declare sub FC_SetTabWidth(byval width_in_spaces as ulong)
declare sub FC_SetRenderCallback(byval callback as function(byval src as SDL_Texture ptr, byval srcrect as SDL_Rect ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval xscale as single, byval yscale as single) as SDL_Rect)
declare function FC_DefaultRenderCallback(byval src as SDL_Texture ptr, byval srcrect as SDL_Rect ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval xscale as single, byval yscale as single) as SDL_Rect
declare function FC_GetNumCacheLevels(byval font as FC_Font ptr) as long
declare function FC_GetGlyphCacheLevel(byval font as FC_Font ptr, byval cache_level as long) as SDL_Texture ptr
declare function FC_SetGlyphCacheLevel(byval font as FC_Font ptr, byval cache_level as long, byval cache_texture as SDL_Texture ptr) as Uint8
declare function FC_UploadGlyphCache(byval font as FC_Font ptr, byval cache_level as long, byval data_surface as SDL_Surface ptr) as Uint8
declare function FC_GetNumCodepoints(byval font as FC_Font ptr) as ulong
declare sub FC_GetCodepoints(byval font as FC_Font ptr, byval result as Uint32 ptr)
declare function FC_GetGlyphData(byval font as FC_Font ptr, byval result as FC_GlyphData ptr, byval codepoint as Uint32) as Uint8
declare function FC_SetGlyphData(byval font as FC_Font ptr, byval codepoint as Uint32, byval glyph_data as FC_GlyphData) as FC_GlyphData ptr
declare function FC_Draw(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawAlign(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval align as FC_AlignEnum, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawScale(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval scale as FC_Scale, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawColor(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval color as SDL_Color, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawEffect(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval effect as FC_Effect, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawBox(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval box as SDL_Rect, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawBoxAlign(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval box as SDL_Rect, byval align as FC_AlignEnum, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawBoxScale(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval box as SDL_Rect, byval scale as FC_Scale, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawBoxColor(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval box as SDL_Rect, byval color as SDL_Color, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawBoxEffect(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval box as SDL_Rect, byval effect as FC_Effect, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawColumn(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval width as Uint16, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawColumnAlign(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval width as Uint16, byval align as FC_AlignEnum, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawColumnScale(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval width as Uint16, byval scale as FC_Scale, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawColumnColor(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval width as Uint16, byval color as SDL_Color, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_DrawColumnEffect(byval font as FC_Font ptr, byval dest as SDL_Renderer ptr, byval x as single, byval y as single, byval width as Uint16, byval effect as FC_Effect, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_GetFilterMode(byval font as FC_Font ptr) as FC_FilterEnum
declare function FC_GetLineHeight(byval font as FC_Font ptr) as Uint16
declare function FC_GetHeight(byval font as FC_Font ptr, byval formatted_text as const zstring ptr, ...) as Uint16
declare function FC_GetWidth(byval font as FC_Font ptr, byval formatted_text as const zstring ptr, ...) as Uint16
declare function FC_GetCharacterOffset(byval font as FC_Font ptr, byval position_index as Uint16, byval column_width as long, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_GetColumnHeight(byval font as FC_Font ptr, byval width as Uint16, byval formatted_text as const zstring ptr, ...) as Uint16
declare function FC_GetAscent(byval font as FC_Font ptr, byval formatted_text as const zstring ptr, ...) as long
declare function FC_GetDescent(byval font as FC_Font ptr, byval formatted_text as const zstring ptr, ...) as long
declare function FC_GetBaseline(byval font as FC_Font ptr) as long
declare function FC_GetSpacing(byval font as FC_Font ptr) as long
declare function FC_GetLineSpacing(byval font as FC_Font ptr) as long
declare function FC_GetMaxWidth(byval font as FC_Font ptr) as Uint16
declare function FC_GetDefaultColor(byval font as FC_Font ptr) as SDL_Color
declare function FC_GetBounds(byval font as FC_Font ptr, byval x as single, byval y as single, byval align as FC_AlignEnum, byval scale as FC_Scale, byval formatted_text as const zstring ptr, ...) as SDL_Rect
declare function FC_InRect(byval x as single, byval y as single, byval input_rect as SDL_Rect) as Uint8
declare function FC_GetPositionFromOffset(byval font as FC_Font ptr, byval x as single, byval y as single, byval column_width as long, byval align as FC_AlignEnum, byval formatted_text as const zstring ptr, ...) as Uint16
declare function FC_GetWrappedText(byval font as FC_Font ptr, byval result as zstring ptr, byval max_result_size as long, byval width as Uint16, byval formatted_text as const zstring ptr, ...) as long
declare sub FC_SetFilterMode(byval font as FC_Font ptr, byval filter as FC_FilterEnum)
declare sub FC_SetSpacing(byval font as FC_Font ptr, byval LetterSpacing as long)
declare sub FC_SetLineSpacing(byval font as FC_Font ptr, byval LineSpacing as long)
declare sub FC_SetDefaultColor(byval font as FC_Font ptr, byval color as SDL_Color)

end extern

'' End of SDL_FontCache.bi
