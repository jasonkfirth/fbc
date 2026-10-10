'' Project: FreeBASIC SDL3 examples
'' File: showfont.bas
'' Purpose: Port upstream SDL3_ttf-3.2.2/examples/showfont.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' showfont:  An example of using the SDL_ttf library with 2D graphics.
'' Copyright (C) 2001-2025 Sam Lantinga <slouken@libsdl.org>
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
'' claim that you wrote the original software. If you use this software
'' in a product, an acknowledgment in the product documentation would be
'' appreciated but is not required.
'' 2. Altered source versions must be plainly marked as such, and must not be
'' misrepresented as being the original software.
'' 3. This notice may not be removed or altered from any source distribution.

#include once "SDL3/SDL.bi"
#include once "smoke.bi"
#include once "SDL3/SDL_ttf.bi"
#include once "crt.bi"

#define DEFAULT_PTSIZE 18.0f
#define DEFAULT_TEXT "The quick brown fox jumped over the lazy dog"
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define MAX_FALLBACKS 4
#define TTF_SHOWFONT_USAGE !"Usage: %s [--textengine surface|renderer] [--solid] [--shaded] [--blended] [-b] [-i] [-u] [-s] [--outline size] [--hintlight|--hintmono|--hintnone] [--nokerning] [--wrap] [--align left|center|right] [--fgcol r,g,b,a] [--bgcol r,g,b,a] [--disable-editbox] [--fallback <font>.ttf>] <font>.ttf [ptsize] [text]\n"

type EditBox
	window_ as SDL_Window ptr
	renderer as SDL_Renderer ptr
	font as TTF_Font ptr
	text as TTF_Text ptr
	rect as SDL_FRect
	has_focus as boolean
	cursor as long
	cursor_length as long
	cursor_visible as boolean
	last_cursor_change as Uint64
	cursor_rect as SDL_FRect
	highlighting as boolean
	highlight1 as long
	highlight2 as long
	composition_start as long
	composition_length as long
	composition_cursor as long
	composition_cursor_length as long
	candidates as TTF_Text ptr
	selected_candidate_start as long
	selected_candidate_length as long
	window_surface as SDL_Surface ptr
end type

enum TextEngine
	TextEngineNone
	TextEngineSurface
	TextEngineRenderer
end enum

enum TextRenderMethod
	TextRenderSolid
	TextRenderShaded
	TextRenderBlended
end enum

type Scene
	done as boolean
	window_ as SDL_Window ptr
	window_surface as SDL_Surface ptr
	renderer as SDL_Renderer ptr
	font as TTF_Font ptr
	caption as TTF_Text ptr
	captionRect as SDL_Rect
	message as SDL_Texture ptr
	messageRect as SDL_FRect
	textEngine as TextEngine
	textRect as SDL_FRect
	edit as EditBox ptr
end type

declare function EditBox_Create cdecl(byval window_ as SDL_Window ptr, byval renderer as SDL_Renderer ptr, byval engine as TTF_TextEngine ptr, byval font as TTF_Font ptr, byval rect as const SDL_FRect ptr) as EditBox ptr
declare sub EditBox_Destroy cdecl(byval edit as EditBox ptr)
declare sub EditBox_SetFocus cdecl(byval edit as EditBox ptr, byval focus as boolean)
declare sub EditBox_Draw cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorLeft cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorRight cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorUp cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorDown cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorBeginningOfLine cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorEndOfLine cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorBeginning cdecl(byval edit as EditBox ptr)
declare sub EditBox_MoveCursorEnd cdecl(byval edit as EditBox ptr)
declare sub EditBox_Backspace cdecl(byval edit as EditBox ptr)
declare sub EditBox_BackspaceToBeginning cdecl(byval edit as EditBox ptr)
declare sub EditBox_DeleteToEnd cdecl(byval edit as EditBox ptr)
declare sub EditBox_Delete cdecl(byval edit as EditBox ptr)
declare sub EditBox_SelectAll cdecl(byval edit as EditBox ptr)
declare function EditBox_DeleteHighlight cdecl(byval edit as EditBox ptr) as boolean
declare sub EditBox_Copy cdecl(byval edit as EditBox ptr)
declare sub EditBox_Cut cdecl(byval edit as EditBox ptr)
declare sub EditBox_Paste cdecl(byval edit as EditBox ptr)
declare sub EditBox_Insert cdecl(byval edit as EditBox ptr, byval text as const zstring ptr)
declare function EditBox_HandleEvent cdecl(byval edit as EditBox ptr, byval event as SDL_Event ptr) as boolean
declare sub DrawScene cdecl(byval scene as Scene ptr)
declare sub AdjustTextOffset cdecl(byval text as TTF_Text ptr, byval xoffset as long, byval yoffset as long)
declare sub HandleKeyDown cdecl(byval scene as Scene ptr, byval event as SDL_Event ptr)
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

sub DrawScene cdecl(byval scene as Scene ptr)
	dim renderer as SDL_Renderer ptr = scene->renderer
	'' Clear the background to background color
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
	SDL_RenderClear(renderer)
	if scene->edit then
		'' Clear the text rect to light gray
		SDL_SetRenderDrawColor(renderer, 204, 204, 204, 255)
		SDL_RenderFillRect(renderer, @(scene->textRect))
		if scene->edit->has_focus then
			dim focusRect as SDL_FRect = scene->textRect
			focusRect.x -= 1
			focusRect.y -= 1
			focusRect.w += 2
			focusRect.h += 2
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
			SDL_RenderRect(renderer, @(focusRect))
		end if
		EditBox_Draw(scene->edit)
	end if
	select case scene->textEngine
		case (TextEngineSurface)
			goto switch_case_1
		case (TextEngineRenderer)
			goto switch_case_2
		case else
			goto switch_case_3
	end select
	switch_case_1:
	scope
		'' Flush the renderer so we can draw directly to the window surface
		SDL_FlushRenderer(renderer)
		TTF_DrawSurfaceText(scene->caption, scene->captionRect.x, scene->captionRect.y, scene->window_surface)
		goto switch_done_4
	end scope
	switch_case_2:
	scope
		TTF_DrawRendererText(scene->caption, cast(single, scene->captionRect.x), cast(single, scene->captionRect.y))
		goto switch_done_4
	end scope
	switch_case_3:
	scope
		do
			do
				if ((((strptr("Unknown text engine") = 0)) = 0)) = 0 then exit do
				static sdl_assert_data as SDL_AssertData = type<SDL_AssertData>(false, 0, strptr(!"!\"Unknown text engine\""), cptr(const zstring ptr, 0), 0, cptr(const zstring ptr, 0), cptr(const SDL_AssertData ptr, 0))
				dim sdl_assert_state as SDL_AssertState = SDL_ReportAssertion(@(sdl_assert_data), __FUNCTION__, strptr("showfont.c"), 117)
				if (sdl_assert_state = SDL_ASSERTION_RETRY) then
					goto loop_continue_6
				else
					if (sdl_assert_state = SDL_ASSERTION_BREAK) then
						SDL_TriggerBreakpoint()
					end if
				end if
				exit do
				loop_continue_6:
			loop
			if ((0)) = 0 then exit do
		loop
		goto switch_done_4
	end scope
	switch_done_4:
	SDL_RenderTexture(renderer, scene->message, cptr(const SDL_FRect ptr, 0), @(scene->messageRect))
	SDL_RenderPresent(renderer)
	SDL3_ExampleSmokeFrame()
	if scene->window_surface then
		SDL_UpdateWindowSurface(scene->window_)
	end if
end sub

sub AdjustTextOffset cdecl(byval text as TTF_Text ptr, byval xoffset as long, byval yoffset as long)
	dim x as long
	dim y as long
	TTF_GetTextPosition(text, @(x), @(y))
	x += xoffset
	y += yoffset
	TTF_SetTextPosition(text, x, y)
end sub

sub HandleKeyDown cdecl(byval scene as Scene ptr, byval event as SDL_Event ptr)
	dim style as long
	dim outline as long
	dim ptsize as single
	select case event->key.key
		case (97u)
			goto switch_case_7
		case (98u)
			goto switch_case_8
		case (105u)
			goto switch_case_9
		case (111u)
			goto switch_case_10
		case (114u)
			goto switch_case_11
		case (115u)
			goto switch_case_12
		case (117u)
			goto switch_case_13
		case (1073741904u)
			goto switch_case_14
		case (1073741903u)
			goto switch_case_15
		case (1073741906u)
			goto switch_case_16
		case (1073741905u)
			goto switch_case_17
		case (27u)
			goto switch_case_18
		case else
			goto switch_case_19
	end select
	switch_case_7:
	scope
		'' Cycle alignment
		select case TTF_GetFontWrapAlignment(scene->font)
			case TTF_HORIZONTAL_ALIGN_LEFT
				goto switch_case_21
			case TTF_HORIZONTAL_ALIGN_CENTER
				goto switch_case_22
			case TTF_HORIZONTAL_ALIGN_RIGHT
				goto switch_case_23
			case else
				goto switch_case_24
		end select
		switch_case_21:
		scope
			TTF_SetFontWrapAlignment(scene->font, TTF_HORIZONTAL_ALIGN_CENTER)
			goto switch_done_25
		end scope
		switch_case_22:
		scope
			TTF_SetFontWrapAlignment(scene->font, TTF_HORIZONTAL_ALIGN_RIGHT)
			goto switch_done_25
		end scope
		switch_case_23:
		scope
			TTF_SetFontWrapAlignment(scene->font, TTF_HORIZONTAL_ALIGN_LEFT)
			goto switch_done_25
		end scope
		switch_case_24:
		scope
			SDL_Log_(strptr("Unknown wrap alignment: %d"), TTF_GetFontWrapAlignment(scene->font))
			goto switch_done_25
		end scope
		switch_done_25:
		goto switch_done_20
	end scope
	switch_case_8:
	scope
		'' Toggle bold style
		style = TTF_GetFontStyle(scene->font)
		if (style and TTF_STYLE_BOLD) then
			style and= (not TTF_STYLE_BOLD)
		else
			style or= TTF_STYLE_BOLD
		end if
		TTF_SetFontStyle(scene->font, style)
		goto switch_done_20
	end scope
	switch_case_9:
	scope
		'' Toggle italic style
		style = TTF_GetFontStyle(scene->font)
		if (style and TTF_STYLE_ITALIC) then
			style and= (not TTF_STYLE_ITALIC)
		else
			style or= TTF_STYLE_ITALIC
		end if
		TTF_SetFontStyle(scene->font, style)
		goto switch_done_20
	end scope
	switch_case_10:
	scope
		'' Toggle scene->font outline
		outline = TTF_GetFontOutline(scene->font)
		if outline then
			outline = 0
		else
			outline = 1
		end if
		TTF_SetFontOutline(scene->font, outline)
		goto switch_done_20
	end scope
	switch_case_11:
	scope
		'' Toggle layout direction
		if ((TTF_GetFontDirection(scene->font) = TTF_DIRECTION_INVALID) orelse (TTF_GetFontDirection(scene->font) = TTF_DIRECTION_LTR)) then
			TTF_SetFontDirection(scene->font, TTF_DIRECTION_RTL)
		else
			if (TTF_GetFontDirection(scene->font) = TTF_DIRECTION_RTL) then
				TTF_SetFontDirection(scene->font, TTF_DIRECTION_LTR)
			else
				if (TTF_GetFontDirection(scene->font) = TTF_DIRECTION_TTB) then
					TTF_SetFontDirection(scene->font, TTF_DIRECTION_BTT)
				else
					if (TTF_GetFontDirection(scene->font) = TTF_DIRECTION_BTT) then
						TTF_SetFontDirection(scene->font, TTF_DIRECTION_TTB)
					end if
				end if
			end if
		end if
		goto switch_done_20
	end scope
	switch_case_12:
	scope
		'' Toggle strike-through style
		style = TTF_GetFontStyle(scene->font)
		if (style and TTF_STYLE_STRIKETHROUGH) then
			style and= (not TTF_STYLE_STRIKETHROUGH)
		else
			style or= TTF_STYLE_STRIKETHROUGH
		end if
		TTF_SetFontStyle(scene->font, style)
		goto switch_done_20
	end scope
	switch_case_13:
	scope
		'' Toggle underline style
		style = TTF_GetFontStyle(scene->font)
		if (style and TTF_STYLE_UNDERLINE) then
			style and= (not TTF_STYLE_UNDERLINE)
		else
			style or= TTF_STYLE_UNDERLINE
		end if
		TTF_SetFontStyle(scene->font, style)
		goto switch_done_20
	end scope
	switch_case_14:
	scope
		if (event->key.mod_ and (SDL_KMOD_CTRL)) then
			AdjustTextOffset(scene->edit->text, (-1), 0)
		end if
		goto switch_done_20
	end scope
	switch_case_15:
	scope
		if (event->key.mod_ and (SDL_KMOD_CTRL)) then
			AdjustTextOffset(scene->edit->text, 1, 0)
		end if
		goto switch_done_20
	end scope
	switch_case_16:
	scope
		if (event->key.mod_ and (SDL_KMOD_CTRL)) then
			AdjustTextOffset(scene->edit->text, 0, (-1))
		else
			'' Increase font size
			ptsize = TTF_GetFontSize(scene->font)
			TTF_SetFontSize(scene->font, (ptsize + 1.0f))
		end if
		goto switch_done_20
	end scope
	switch_case_17:
	scope
		if (event->key.mod_ and (SDL_KMOD_CTRL)) then
			AdjustTextOffset(scene->edit->text, 0, 1)
		else
			'' Decrease font size
			ptsize = TTF_GetFontSize(scene->font)
			TTF_SetFontSize(scene->font, (ptsize - 1.0f))
		end if
		goto switch_done_20
	end scope
	switch_case_18:
	scope
		scene->done = true
		goto switch_done_20
	end scope
	switch_case_19:
	scope
		goto switch_done_20
	end scope
	switch_done_20:
end sub

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	dim argv0 as zstring ptr = argv[0]
	dim font as TTF_Font ptr = cptr(TTF_Font ptr, 0)
	dim text as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
	dim scene as Scene
	dim ptsize as single
	dim i as long
	dim white as SDL_Color = type<SDL_Color>(255, 255, 255, SDL_ALPHA_OPAQUE)
	dim black as SDL_Color = type<SDL_Color>(0, 0, 0, SDL_ALPHA_OPAQUE)
	dim forecol as SDL_Color ptr
	dim backcol as SDL_Color ptr
	dim event as SDL_Event
	dim engine as TTF_TextEngine ptr = cptr(TTF_TextEngine ptr, 0)
	dim rendermethod as TextRenderMethod = TextRenderShaded
	dim renderstyle as long = TTF_STYLE_NORMAL
	dim outline as long = 0
	dim hinting as long = TTF_HINTING_NORMAL
	dim kerning as long = 1
	dim wrap as boolean = false
	dim align as TTF_HorizontalAlignment = TTF_HORIZONTAL_ALIGN_LEFT
	dim editbox as boolean = true
	dim dump as boolean = false
	dim message as zstring ptr
	dim string_(0 to 127) as byte
	dim num_fallbacks as long = 0
	dim fallback_font_files(0 to 3) as const zstring ptr
	dim fallback_fonts(0 to 3) as TTF_Font ptr
	dim result as long = 0
	SDL_memset(cptr(any ptr, @((scene))), 0, sizeof(((scene))))
	scene.textEngine = TextEngineRenderer
	SDL_memset(cptr(any ptr, @fallback_fonts(0)), 0, (sizeof(TTF_Font ptr) * 4))
	'' Default is black and white
	forecol = @(black)
	backcol = @(white)
	scope
		i = 1
		do while (argv[i] andalso (cptr(byte ptr, argv[i])[0] = 45))
			if ((SDL_strcmp(argv[i], strptr("--fallback")) = 0) andalso argv[(i + 1)]) then
				i += 1
				if (num_fallbacks < 4) then
					dim expression_value_27 as long = num_fallbacks
					num_fallbacks += 1
					fallback_font_files(expression_value_27) = argv[i]
				else
					SDL_Log_(strptr("Too many fallback fonts (maximum = %d)"), 4)
					return (1)
				end if
			else
				if ((SDL_strcmp(argv[i], strptr("--textengine")) = 0) andalso argv[(i + 1)]) then
					i += 1
					if (SDL_strcmp(argv[i], strptr("surface")) = 0) then
						scene.textEngine = TextEngineSurface
					else
						if (SDL_strcmp(argv[i], strptr("renderer")) = 0) then
							scene.textEngine = TextEngineRenderer
						else
							SDL_Log_(strptr(TTF_SHOWFONT_USAGE), argv0)
							return (1)
						end if
					end if
				else
					if (SDL_strcmp(argv[i], strptr("--solid")) = 0) then
						rendermethod = TextRenderSolid
					else
						if (SDL_strcmp(argv[i], strptr("--shaded")) = 0) then
							rendermethod = TextRenderShaded
						else
							if (SDL_strcmp(argv[i], strptr("--blended")) = 0) then
								rendermethod = TextRenderBlended
							else
								if (SDL_strcmp(argv[i], strptr("-b")) = 0) then
									renderstyle or= TTF_STYLE_BOLD
								else
									if (SDL_strcmp(argv[i], strptr("-i")) = 0) then
										renderstyle or= TTF_STYLE_ITALIC
									else
										if (SDL_strcmp(argv[i], strptr("-u")) = 0) then
											renderstyle or= TTF_STYLE_UNDERLINE
										else
											if (SDL_strcmp(argv[i], strptr("-s")) = 0) then
												renderstyle or= TTF_STYLE_STRIKETHROUGH
											else
												if ((SDL_strcmp(argv[i], strptr("--outline")) = 0) andalso argv[(i + 1)]) then
													i += 1
													if (SDL_sscanf(argv[i], strptr("%d"), @(outline)) <> 1) then
														SDL_Log_(strptr(TTF_SHOWFONT_USAGE), argv0)
														return (1)
													end if
												else
													if (SDL_strcmp(argv[i], strptr("--hintlight")) = 0) then
														hinting = TTF_HINTING_LIGHT
													else
														if (SDL_strcmp(argv[i], strptr("--hintmono")) = 0) then
															hinting = TTF_HINTING_MONO
														else
															if (SDL_strcmp(argv[i], strptr("--hintnone")) = 0) then
																hinting = TTF_HINTING_NONE
															else
																if (SDL_strcmp(argv[i], strptr("--nokerning")) = 0) then
																	kerning = 0
																else
																	if (SDL_strcmp(argv[i], strptr("--wrap")) = 0) then
																		wrap = true
																	else
																		if ((SDL_strcmp(argv[i], strptr("--align")) = 0) andalso argv[(i + 1)]) then
																			i += 1
																			if (SDL_strcmp(argv[i], strptr("left")) = 0) then
																				align = TTF_HORIZONTAL_ALIGN_LEFT
																			else
																				if (SDL_strcmp(argv[i], strptr("center")) = 0) then
																					align = TTF_HORIZONTAL_ALIGN_CENTER
																				else
																					if (SDL_strcmp(argv[i], strptr("right")) = 0) then
																						align = TTF_HORIZONTAL_ALIGN_RIGHT
																					else
																						SDL_Log_(strptr(TTF_SHOWFONT_USAGE), argv0)
																						return (1)
																					end if
																				end if
																			end if
																		else
																			if ((SDL_strcmp(argv[i], strptr("--fgcol")) = 0) andalso argv[(i + 1)]) then
																				dim r as long
																				dim g as long
																				dim b as long
																				dim a as long = SDL_ALPHA_OPAQUE
																				i += 1
																				if (SDL_sscanf(argv[i], strptr("%d,%d,%d,%d"), @(r), @(g), @(b), @(a)) < 3) then
																					SDL_Log_(strptr(TTF_SHOWFONT_USAGE), argv0)
																					return (1)
																				end if
																				forecol->r = cast(Uint8, r)
																				forecol->g = cast(Uint8, g)
																				forecol->b = cast(Uint8, b)
																				forecol->a = cast(Uint8, a)
																			else
																				if ((SDL_strcmp(argv[i], strptr("--bgcol")) = 0) andalso argv[(i + 1)]) then
																					dim r as long
																					dim g as long
																					dim b as long
																					dim a as long = SDL_ALPHA_OPAQUE
																					i += 1
																					if (SDL_sscanf(argv[i], strptr("%d,%d,%d,%d"), @(r), @(g), @(b), @(a)) < 3) then
																						SDL_Log_(strptr(TTF_SHOWFONT_USAGE), argv0)
																						return (1)
																					end if
																					backcol->r = cast(Uint8, r)
																					backcol->g = cast(Uint8, g)
																					backcol->b = cast(Uint8, b)
																					backcol->a = cast(Uint8, a)
																				else
																					if (SDL_strcmp(argv[i], strptr("--disable-editbox")) = 0) then
																						editbox = false
																					else
																						if (SDL_strcmp(argv[i], strptr("--dump")) = 0) then
																							dump = true
																						else
																							SDL_Log_(strptr(TTF_SHOWFONT_USAGE), argv0)
																							return (1)
																						end if
																					end if
																				end if
																			end if
																		end if
																	end if
																end if
															end if
														end if
													end if
												end if
											end if
										end if
									end if
								end if
							end if
						end if
					end if
				end if
			end if
			i += 1
		loop
	end scope
	argv += i
	argc -= i
	'' Check usage
	if (argv[0] = 0) then
		SDL_Log_(strptr(TTF_SHOWFONT_USAGE), argv0)
		return (1)
	end if
	'' Initialize the TTF library
	if (TTF_Init() = 0) then
		SDL_Log_(strptr("Couldn't initialize TTF: %s"), SDL_GetError())
		result = 2
		goto done
	end if
	'' Open the font file with the requested point size
	ptsize = 0.0f
	if (argc > 1) then
		ptsize = cast(single, SDL_atof(argv[1]))
	end if
	if (ptsize = 0.0f) then
		i = 2
		ptsize = 18.0f
	else
		i = 3
	end if
	font = TTF_OpenFont(argv[0], ptsize)
	if (font = cptr(TTF_Font ptr, (cptr(any ptr, 0)))) then
		SDL_Log_(strptr("Couldn't load %g pt font from %s: %s"), cast(double, ptsize), argv[0], SDL_GetError())
		result = 2
		goto done
	end if
	TTF_SetFontStyle(font, renderstyle)
	TTF_SetFontOutline(font, outline)
	TTF_SetFontKerning(font, cast(boolean, kerning))
	TTF_SetFontHinting(font, hinting)
	TTF_SetFontWrapAlignment(font, align)
	scene.font = font
	scope
		i = 0
		do while (i < num_fallbacks)
			fallback_fonts(i) = TTF_OpenFont(fallback_font_files(i), ptsize)
			if (fallback_fonts(i) = 0) then
				SDL_Log_(strptr("Couldn't load %g pt font from %s: %s"), cast(double, ptsize), fallback_font_files(i), SDL_GetError())
				result = 2
				goto done
			end if
			TTF_AddFallbackFont(font, fallback_fonts(i))
			i += 1
		loop
	end scope
	if dump then
		scope
			i = 48
			do while (i < 123)
				dim glyph as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
				glyph = TTF_RenderGlyph_Shaded(font, i, (*forecol), (*backcol))
				if glyph then
					dim outname(0 to 63) as byte
					SDL_snprintf(@outname(0), (sizeof(byte) * 64), strptr("glyph-%d.bmp"), i)
					SDL_SaveBMP(glyph, @outname(0))
				end if
				i += 1
			loop
		end scope
		result = 0
		goto done
	end if
	'' Create a window
	scene.window_ = SDL_CreateWindow(strptr("showfont demo"), 640, 480, 0)
	if (scene.window_ = 0) then
		SDL_Log_(strptr("SDL_CreateWindow() failed: %s"), SDL_GetError())
		result = 2
		goto done
	end if
	if (scene.textEngine = TextEngineSurface) then
		scene.window_surface = SDL_GetWindowSurface(scene.window_)
		if (scene.window_surface = 0) then
			SDL_Log_(strptr("SDL_CreateWindowSurface() failed: %s"), SDL_GetError())
			result = 2
			goto done
		end if
		SDL_SetWindowSurfaceVSync(scene.window_, 1)
		scene.renderer = SDL_CreateSoftwareRenderer(scene.window_surface)
	else
		scene.renderer = SDL_CreateRenderer(scene.window_, cptr(const zstring ptr, 0))
		if scene.renderer then
			SDL_SetRenderVSync(scene.renderer, 1)
		end if
	end if
	if (scene.renderer = 0) then
		SDL_Log_(strptr("SDL_CreateRenderer() failed: %s"), SDL_GetError())
		result = 2
		goto done
	end if
	select case scene.textEngine
		case (TextEngineSurface)
			goto switch_case_30
		case (TextEngineRenderer)
			goto switch_case_31
		case else
			goto switch_case_32
	end select
	switch_case_30:
	scope
		engine = TTF_CreateSurfaceTextEngine()
		if (engine = 0) then
			SDL_Log_(strptr("Couldn't create surface text engine: %s"), SDL_GetError())
			result = 2
			goto done
		end if
		goto switch_done_33
	end scope
	switch_case_31:
	scope
		engine = TTF_CreateRendererTextEngine(scene.renderer)
		if (engine = 0) then
			SDL_Log_(strptr("Couldn't create renderer text engine: %s"), SDL_GetError())
			result = 2
			goto done
		end if
		goto switch_done_33
	end scope
	switch_case_32:
	scope
		goto switch_done_33
	end scope
	switch_done_33:
	'' Show which font file we're looking at
	SDL_snprintf(@string_(0), (sizeof(byte) * 128), strptr("Font file: %s"), argv[0])
	'' possible overflow
	scene.caption = TTF_CreateText(engine, font, @string_(0), 0)
	TTF_SetTextColor(scene.caption, forecol->r, forecol->g, forecol->b, forecol->a)
	scene.captionRect.x = 4
	scene.captionRect.y = 4
	TTF_GetTextSize(scene.caption, @(scene.captionRect.w), @(scene.captionRect.h))
	'' Render and center the message
	if (argc > 2) then
		message = argv[2]
	else
		message = strptr("The quick brown fox jumped over the lazy dog")
	end if
	select case rendermethod
		case (TextRenderSolid)
			goto switch_case_34
		case (TextRenderShaded)
			goto switch_case_35
		case (TextRenderBlended)
			goto switch_case_36
		case else
			goto switch_done_37
	end select
	switch_case_34:
	scope
		if wrap then
			text = TTF_RenderText_Solid_Wrapped(font, message, 0, (*forecol), 0)
		else
			text = TTF_RenderText_Solid(font, message, 0, (*forecol))
		end if
		goto switch_done_37
	end scope
	switch_case_35:
	scope
		if wrap then
			text = TTF_RenderText_Shaded_Wrapped(font, message, 0, (*forecol), (*backcol), 0)
		else
			text = TTF_RenderText_Shaded(font, message, 0, (*forecol), (*backcol))
		end if
		goto switch_done_37
	end scope
	switch_case_36:
	scope
		if wrap then
			text = TTF_RenderText_Blended_Wrapped(font, message, 0, (*forecol), 0)
		else
			text = TTF_RenderText_Blended(font, message, 0, (*forecol))
		end if
		goto switch_done_37
	end scope
	switch_done_37:
	if (text = cptr(SDL_Surface ptr, (cptr(any ptr, 0)))) then
		SDL_Log_(strptr("Couldn't render text: %s"), SDL_GetError())
		result = 2
		goto done
	end if
	scene.messageRect.x = cast(single, ((((640 - text->w)) \ 2)))
	scene.messageRect.y = cast(single, ((((480 - text->h)) \ 2)))
	scene.messageRect.w = cast(single, text->w)
	scene.messageRect.h = cast(single, text->h)
	scene.message = SDL_CreateTextureFromSurface(scene.renderer, text)
	SDL_Log_(strptr("Font is generally %d big, and string is %d big"), TTF_GetFontHeight(font), text->h)
	if editbox then
		scene.textRect.x = 8.0f
		scene.textRect.y = ((scene.captionRect.y + scene.captionRect.h) + 4.0f)
		scene.textRect.w = ((640 \ 2) - (scene.textRect.x * 2))
		scene.textRect.h = ((scene.messageRect.y - scene.textRect.y) - 16.0f)
		dim editRect as SDL_FRect = scene.textRect
		editRect.x += 4.0f
		editRect.y += 4.0f
		editRect.w -= 8.0f
		editRect.w -= 8.0f
		scene.edit = EditBox_Create(scene.window_, scene.renderer, engine, font, @(editRect))
		if scene.edit then
			TTF_SetTextColor(scene.edit->text, forecol->r, forecol->g, forecol->b, forecol->a)
			EditBox_Insert(scene.edit, message)
		end if
	end if
	'' Wait for a keystroke, and blit text on mouse press
	do
		if ((scene.done = 0)) = 0 then exit do
		do
			if (SDL_PollEvent(@(event))) = 0 then exit do
			SDL_ConvertEventToRenderCoordinates(scene.renderer, @(event))
			select case event.type
				case SDL_EVENT_MOUSE_BUTTON_DOWN
					goto switch_case_40
				case SDL_EVENT_KEY_DOWN
					goto switch_case_41
				case SDL_EVENT_QUIT
					goto switch_case_42
				case else
					goto switch_case_43
			end select
			switch_case_40:
			scope
				if (EditBox_HandleEvent(scene.edit, @(event)) = 0) then
					scene.messageRect.x = ((event.button.x - (text->w \ 2)))
					scene.messageRect.y = ((event.button.y - (text->h \ 2)))
					scene.messageRect.w = cast(single, text->w)
					scene.messageRect.h = cast(single, text->h)
				end if
				goto switch_done_44
			end scope
			switch_case_41:
			scope
				if (EditBox_HandleEvent(scene.edit, @(event)) = 0) then
					HandleKeyDown(@(scene), @(event))
				end if
				goto switch_done_44
			end scope
			switch_case_42:
			scope
				scene.done = true
				goto switch_done_44
			end scope
			switch_case_43:
			scope
				EditBox_HandleEvent(scene.edit, @(event))
				goto switch_done_44
			end scope
			switch_done_44:
		loop
		DrawScene(@(scene))
	loop
	result = 0
	done:
	SDL_DestroySurface(text)
	EditBox_Destroy(scene.edit)
	TTF_DestroyText(scene.caption)
	scope
		i = 0
		do while (i < num_fallbacks)
			TTF_CloseFont(fallback_fonts(i))
			i += 1
		loop
	end scope
	TTF_CloseFont(font)
	select case scene.textEngine
		case (TextEngineSurface)
			goto switch_case_46
		case (TextEngineRenderer)
			goto switch_case_47
		case else
			goto switch_case_48
	end select
	switch_case_46:
	scope
		TTF_DestroySurfaceTextEngine(engine)
		goto switch_done_49
	end scope
	switch_case_47:
	scope
		TTF_DestroyRendererTextEngine(engine)
		goto switch_done_49
	end scope
	switch_case_48:
	scope
		goto switch_done_49
	end scope
	switch_done_49:
	SDL_DestroyTexture(scene.message)
	TTF_Quit()
	SDL_Quit()
	return result
end function

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of showfont.bas
