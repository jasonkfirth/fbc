'' Project: FreeBASIC SDL addon examples
'' File: rtf-font-engine.bi
'' Purpose: Supply the font callbacks required by the RTF renderer.
'' Responsibilities: Measure UTF-8 text, render glyphs, and own each callback font.
'' This file intentionally does NOT contain: document loading or window creation.

#pragma once
#if SDL_ADDON_API = 3
	#include once "SDL3/SDL_rtf.bi"
	#include once "SDL3/SDL_ttf.bi"
#elseif SDL_ADDON_API = 2
	#include once "SDL2/SDL_rtf.bi"
	#include once "SDL2/SDL_ttf.bi"
#else
	#include once "SDL/SDL_rtf.bi"
	#include once "SDL/SDL_ttf.bi"
#endif

'' SDL_rtf invokes these callbacks on the same thread as RTF_Load/RTF_Render.
'' The font filename remains alive until RTF_FreeContext releases its fonts.
dim shared rtf_fontfile as string

private function rtf_create_font cdecl(byval font_name as const zstring ptr, byval family as RTF_FontFamily, _
	byval charset as long, byval point_size as long, byval style as long) as any ptr
	if point_size < 1 or point_size > 256 then return 0
	dim font as TTF_Font ptr = TTF_OpenFont(strptr(rtf_fontfile), point_size)
	if font <> 0 then TTF_SetFontStyle(font, style)
	return font
end function

private function rtf_line_spacing cdecl(byval font as any ptr) as long
	#if SDL_ADDON_API = 3
		return TTF_GetFontLineSkip(font)
	#else
		return TTF_FontLineSkip(font)
	#endif
end function

private function rtf_offsets cdecl(byval font as any ptr, byval text as const zstring ptr, _
	byval byte_offsets as long ptr, byval pixel_offsets as long ptr, byval max_offsets as long) as long
	if text = 0 or byte_offsets = 0 or pixel_offsets = 0 or max_offsets <= 0 then return 0
	dim i as long, bytes as long, pixels as long
	dim cursor as const Uint8 ptr = cptr(const Uint8 ptr, text)
	while *cursor <> 0 and i < max_offsets
		byte_offsets[i] = bytes
		pixel_offsets[i] = pixels
		i += 1
		dim codepoint as Uint32 = *cursor
		dim count as long = 1
		if codepoint >= &hC2 and codepoint <= &hDF then
			count = 2 : codepoint and= &h1F
		elseif codepoint >= &hE0 and codepoint <= &hEF then
			count = 3 : codepoint and= &h0F
		elseif codepoint >= &hF0 and codepoint <= &hF4 then
			count = 4 : codepoint and= &h07
		end if
		for part as long = 1 to count - 1
			if (cursor[part] and &hC0) <> &h80 then return 0
			codepoint = (codepoint shl 6) or (cursor[part] and &h3F)
		next
		dim advance as long
		#if SDL_ADDON_API = 3
			if not TTF_GetGlyphMetrics(font, codepoint, 0, 0, 0, 0, @advance) then return 0
		#else
			if codepoint > &hFFFF then codepoint = &hFFFD
			if TTF_GlyphMetrics(font, codepoint, 0, 0, 0, 0, @advance) <> 0 then return 0
		#endif
		cursor += count
		bytes += count
		pixels += advance
	wend
	if i < max_offsets then
		byte_offsets[i] = bytes
		pixel_offsets[i] = pixels
	end if
	return i
end function

#if SDL_ADDON_API = 1
private function rtf_render_text cdecl(byval font as any ptr, byval text as const zstring ptr, _
	byval fg as SDL_Color) as SDL_Surface ptr
	return TTF_RenderUTF8_Blended(font, text, fg)
end function
#else
private function rtf_render_text cdecl(byval font as any ptr, byval renderer as SDL_Renderer ptr, _
	byval text as const zstring ptr, byval fg as SDL_Color) as SDL_Texture ptr
	#if SDL_ADDON_API = 3
		dim surface as SDL_Surface ptr = TTF_RenderText_Blended(font, text, 0, fg)
	#else
		dim surface as SDL_Surface ptr = TTF_RenderUTF8_Blended(font, text, fg)
	#endif
	if surface = 0 then return 0
	dim texture as SDL_Texture ptr = SDL_CreateTextureFromSurface(renderer, surface)
	#if SDL_ADDON_API = 3
		SDL_DestroySurface(surface)
	#else
		SDL_FreeSurface(surface)
	#endif
	return texture
end function
#endif

private sub rtf_free_font cdecl(byval font as any ptr)
	TTF_CloseFont(font)
end sub

'' End of rtf-font-engine.bi
