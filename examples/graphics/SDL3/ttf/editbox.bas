'' Project: FreeBASIC SDL3 examples
'' File: editbox.bas
'' Purpose: Port upstream SDL3_ttf-3.2.2/examples/editbox.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>
''
'' This software is provided 'as-is', without any express or implied
'' warranty.  In no event will the authors be held liable for any damages
'' arising from the use of this software.
''
'' Permission is granted to anyone to use this software for any purpose,
'' including commercial applications, and to alter it and redistribute it
'' freely.

#include once "SDL3/SDL.bi"
#include once "SDL3/SDL_ttf.bi"

#define CURSOR_BLINK_INTERVAL_MS 500

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
declare sub DrawText cdecl(byval edit as EditBox ptr, byval text as TTF_Text ptr, byval x as single, byval y as single)
declare function GetHighlightExtents cdecl(byval edit as EditBox ptr, byval marker as long ptr, byval length as long ptr) as boolean
declare sub ResetComposition cdecl(byval edit as EditBox ptr)
declare function UTF8ByteLength cdecl(byval text as const zstring ptr, byval num_codepoints as long) as long
declare sub HandleComposition cdecl(byval edit as EditBox ptr, byval event as const SDL_TextEditingEvent ptr)
declare sub CancelComposition cdecl(byval edit as EditBox ptr)
declare sub DrawComposition cdecl(byval edit as EditBox ptr)
declare sub DrawCompositionCursor cdecl(byval edit as EditBox ptr)
declare sub ClearCandidates cdecl(byval edit as EditBox ptr)
declare sub SaveCandidates cdecl(byval edit as EditBox ptr, byval event as const SDL_Event ptr)
declare sub DrawCandidates cdecl(byval edit as EditBox ptr)
declare sub UpdateTextInputArea cdecl(byval edit as EditBox ptr)
declare sub DrawCursor cdecl(byval edit as EditBox ptr)
declare function GetCursorTextIndex cdecl(byval x as long, byval substring as const TTF_SubString ptr) as long
declare sub SetCursorPosition cdecl(byval edit as EditBox ptr, byval position as long)
declare sub MoveCursorIndex cdecl(byval edit as EditBox ptr, byval direction as long)
declare function HandleMouseDown cdecl(byval edit as EditBox ptr, byval x as single, byval y as single) as boolean
declare function HandleMouseMotion cdecl(byval edit as EditBox ptr, byval x as single, byval y as single) as boolean
declare function HandleMouseUp cdecl(byval edit as EditBox ptr, byval x as single, byval y as single) as boolean

sub DrawText cdecl(byval edit as EditBox ptr, byval text as TTF_Text ptr, byval x as single, byval y as single)
	scope
		if edit->window_surface then
			scope
				'' Flush the renderer so we can draw directly to the window surface
				SDL_FlushRenderer(edit->renderer)
				TTF_DrawSurfaceText(text, cast(long, SDL_roundf(x)), cast(long, SDL_roundf(y)), edit->window_surface)
				exit sub
			end scope
		end if
		'' TEST_SURFACE_ENGINE
		TTF_DrawRendererText(text, x, y)
	end scope
end sub

function GetHighlightExtents cdecl(byval edit as EditBox ptr, byval marker as long ptr, byval length as long ptr) as boolean
	scope
		if ((edit->highlight1 >= 0) andalso (edit->highlight2 >= 0)) then
			scope
				dim marker1 as long = (iif((((edit->highlight1) < (edit->highlight2))), (edit->highlight1), (edit->highlight2)))
				dim marker2 as long = (iif((((edit->highlight1) > (edit->highlight2))), (edit->highlight1), (edit->highlight2)))
				if (marker2 > marker1) then
					scope
						(*marker) = marker1
						(*length) = (marker2 - marker1)
						return true
					end scope
				end if
			end scope
		end if
		return false
	end scope
end function

sub ResetComposition cdecl(byval edit as EditBox ptr)
	scope
		edit->composition_start = 0
		edit->composition_length = 0
		edit->composition_cursor = 0
		edit->composition_cursor_length = 0
	end scope
end sub

function UTF8ByteLength cdecl(byval text as const zstring ptr, byval num_codepoints as long) as long
	scope
		dim start as const zstring ptr = text
		do
			if ((num_codepoints > 0)) = 0 then exit do
			scope
				dim ch as Uint32 = SDL_StepUTF8(@(text), cptr(uinteger ptr, 0))
				if (ch = 0) then
					scope
						exit do
					end scope
				end if
				num_codepoints -= 1
			end scope
		loop
		return cast(long, cast(uinteger, ((text - start))))
	end scope
end function

sub HandleComposition cdecl(byval edit as EditBox ptr, byval event as const SDL_TextEditingEvent ptr)
	scope
		EditBox_DeleteHighlight(edit)
		if (edit->composition_length > 0) then
			scope
				TTF_DeleteTextString(edit->text, edit->composition_start, edit->composition_length)
				ResetComposition(edit)
			end scope
		end if
		dim length as long = cast(long, SDL_strlen(event->text))
		if (length > 0) then
			scope
				edit->composition_start = edit->cursor
				edit->composition_length = length
				TTF_InsertTextString(edit->text, edit->composition_start, event->text, edit->composition_length)
				if ((event->start > 0) orelse (event->length > 0)) then
					scope
						edit->composition_cursor = UTF8ByteLength(@(cptr(byte ptr, edit->text->text)[edit->composition_start]), event->start)
						edit->composition_cursor_length = UTF8ByteLength(@(cptr(byte ptr, edit->text->text)[(edit->composition_start + edit->composition_cursor)]), event->length)
					end scope
				else
					scope
						edit->composition_cursor = length
						edit->composition_cursor_length = 0
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub CancelComposition cdecl(byval edit as EditBox ptr)
	scope
		ResetComposition(edit)
		SDL_ClearComposition(edit->window_)
	end scope
end sub

sub DrawComposition cdecl(byval edit as EditBox ptr)
	scope
		'' Draw an underline under the composed text
		dim renderer as SDL_Renderer ptr = edit->renderer
		dim font_height as long = TTF_GetFontHeight(edit->font)
		dim substrings as TTF_SubString ptr ptr = TTF_GetTextSubStringsForRange(edit->text, edit->composition_start, edit->composition_length, cptr(long ptr, 0))
		if substrings then
			scope
				scope
					dim i as long = 0
					do while substrings[i]
						scope
							dim rect as SDL_FRect
							SDL_RectToFRect(@(substrings[i]->rect), @(rect))
							rect.x += edit->rect.x
							rect.y += ((edit->rect.y + font_height))
							rect.h = 1.0f
							SDL_RenderFillRect(renderer, @(rect))
						end scope
						i += 1
					loop
				end scope
				SDL_free(cptr(any ptr, substrings))
			end scope
		end if
		'' Thicken the underline under the active clause in the composed text
		if (edit->composition_cursor_length > 0) then
			scope
				substrings = TTF_GetTextSubStringsForRange(edit->text, (edit->composition_start + edit->composition_cursor), edit->composition_cursor_length, cptr(long ptr, 0))
				if substrings then
					scope
						scope
							dim i as long = 0
							do while substrings[i]
								scope
									dim rect as SDL_FRect
									SDL_RectToFRect(@(substrings[i]->rect), @(rect))
									rect.x += edit->rect.x
									rect.y += (((edit->rect.y + font_height)) - 1)
									rect.h = 1.0f
									SDL_RenderFillRect(renderer, @(rect))
								end scope
								i += 1
							loop
						end scope
						SDL_free(cptr(any ptr, substrings))
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub DrawCompositionCursor cdecl(byval edit as EditBox ptr)
	scope
		dim renderer as SDL_Renderer ptr = edit->renderer
		if (edit->composition_cursor_length = 0) then
			scope
				dim cursor as TTF_SubString
				if TTF_GetTextSubString(edit->text, (edit->composition_start + edit->composition_cursor), @(cursor)) then
					scope
						dim rect as SDL_FRect
						SDL_RectToFRect(@(cursor.rect), @(rect))
						if (((cursor.flags and TTF_SUBSTRING_DIRECTION_MASK)) = TTF_DIRECTION_RTL) then
							scope
								rect.x += cursor.rect.w
							end scope
						end if
						rect.x += edit->rect.x
						rect.y += edit->rect.y
						rect.w = 1.0f
						SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
						SDL_RenderFillRect(renderer, @(rect))
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub ClearCandidates cdecl(byval edit as EditBox ptr)
	scope
		if edit->candidates then
			scope
				TTF_DestroyText(edit->candidates)
				edit->candidates = cptr(TTF_Text ptr, 0)
			end scope
		end if
		edit->selected_candidate_start = 0
		edit->selected_candidate_length = 0
	end scope
end sub

sub SaveCandidates cdecl(byval edit as EditBox ptr, byval event as const SDL_Event ptr)
	scope
		dim i as long
		ClearCandidates(edit)
		dim horizontal as boolean = event->edit_candidates.horizontal
		dim num_candidates as long = event->edit_candidates.num_candidates
		dim selected_candidate as long = event->edit_candidates.selected_candidate
		'' Calculate the length of the candidates text
		dim length as uinteger = 0
		scope
			i = 0
			do while (i < num_candidates)
				scope
					if horizontal then
						scope
							if (i > 0) then
								scope
									length += 1
								end scope
							end if
						end scope
					end if
					length += SDL_strlen(event->edit_candidates.candidates[i])
					if (horizontal = 0) then
						scope
							length += 1
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		if (length = 0) then
			scope
				exit sub
			end scope
		end if
		length += 1
		'' For null terminator
		dim candidate_text as zstring ptr = cptr(zstring ptr, SDL_malloc(length))
		if (candidate_text = 0) then
			scope
				exit sub
			end scope
		end if
		dim dst as zstring ptr = candidate_text
		scope
			i = 0
			do while (i < num_candidates)
				scope
					if horizontal then
						scope
							if (i > 0) then
								scope
									dim expression_value_6 as zstring ptr = dst
									dst += 1
									(*cptr(byte ptr, expression_value_6)) = 32
								end scope
							end if
						end scope
					end if
					dim length as long = cast(long, SDL_strlen(event->edit_candidates.candidates[i]))
					if (i = selected_candidate) then
						scope
							edit->selected_candidate_start = cast(long, cast(uinteger, ((dst - candidate_text))))
							edit->selected_candidate_length = length
						end scope
					end if
					SDL_memcpy(cptr(any ptr, dst), cptr(const any ptr, event->edit_candidates.candidates[i]), length)
					dst += length
					if (horizontal = 0) then
						scope
							dim expression_value_7 as zstring ptr = dst
							dst += 1
							(*cptr(byte ptr, expression_value_7)) = 10
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		(*cptr(byte ptr, dst)) = 0
		edit->candidates = TTF_CreateText(TTF_GetTextEngine(edit->text), edit->font, candidate_text, 0)
		SDL_free(cptr(any ptr, candidate_text))
		if edit->candidates then
			scope
				dim r as single
				dim g as single
				dim b as single
				dim a as single
				TTF_GetTextColorFloat(edit->text, @(r), @(g), @(b), @(a))
				TTF_SetTextColorFloat(edit->candidates, r, g, b, a)
			end scope
		else
			scope
				ClearCandidates(edit)
			end scope
		end if
	end scope
end sub

sub DrawCandidates cdecl(byval edit as EditBox ptr)
	scope
		dim renderer as SDL_Renderer ptr = edit->renderer
		dim safe_rect as SDL_Rect
		dim candidates_rect as SDL_FRect
		dim candidates_w as long
		dim candidates_h as long
		dim x as single
		dim y as single
		'' Position the candidate window
		dim cursor as TTF_SubString
		dim offset as long = edit->composition_start
		if (edit->composition_cursor_length > 0) then
			scope
				'' Place the candidates at the active clause
				offset += edit->composition_cursor
			end scope
		end if
		if (TTF_GetTextSubString(edit->text, offset, @(cursor)) = 0) then
			scope
				exit sub
			end scope
		end if
		SDL_GetRenderSafeArea(renderer, @(safe_rect))
		TTF_GetTextSize(edit->candidates, @(candidates_w), @(candidates_h))
		candidates_rect.x = (edit->rect.x + cursor.rect.x)
		candidates_rect.y = (((edit->rect.y + cursor.rect.y) + cursor.rect.h) + 2.0f)
		candidates_rect.w = ((((1.0f + 2.0f) + candidates_w) + 2.0f) + 1.0f)
		candidates_rect.h = ((((1.0f + 2.0f) + candidates_h) + 2.0f) + 1.0f)
		if (((candidates_rect.x + candidates_rect.w)) > safe_rect.w) then
			scope
				candidates_rect.x = ((safe_rect.w - candidates_rect.w))
				if (candidates_rect.x < 0.0f) then
					scope
						candidates_rect.x = 0.0f
					end scope
				end if
			end scope
		end if
		'' Draw the candidate background
		SDL_SetRenderDrawColor(renderer, 170, 170, 170, 255)
		SDL_RenderFillRect(renderer, @(candidates_rect))
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		SDL_RenderRect(renderer, @(candidates_rect))
		'' Draw the candidates
		x = (candidates_rect.x + 3.0f)
		y = (candidates_rect.y + 3.0f)
		DrawText(edit, edit->candidates, x, y)
		'' Underline the selected candidate
		if (edit->selected_candidate_length > 0) then
			scope
				dim font_height as long = TTF_GetFontHeight(edit->font)
				dim substrings as TTF_SubString ptr ptr = TTF_GetTextSubStringsForRange(edit->candidates, edit->selected_candidate_start, edit->selected_candidate_length, cptr(long ptr, 0))
				if substrings then
					scope
						scope
							dim i as long = 0
							do while substrings[i]
								scope
									dim rect as SDL_FRect
									SDL_RectToFRect(@(substrings[i]->rect), @(rect))
									rect.x += x
									rect.y += ((y + font_height))
									rect.h = 1.0f
									SDL_RenderFillRect(renderer, @(rect))
								end scope
								i += 1
							loop
						end scope
						SDL_free(cptr(any ptr, substrings))
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub UpdateTextInputArea cdecl(byval edit as EditBox ptr)
	scope
		'' Convert the text input area and cursor into window coordinates
		dim renderer as SDL_Renderer ptr = edit->renderer
		dim window_edit_rect_min as SDL_FPoint
		dim window_edit_rect_max as SDL_FPoint
		dim window_cursor as SDL_FPoint
		if (((SDL_RenderCoordinatesToWindow(renderer, edit->rect.x, edit->rect.y, @(window_edit_rect_min.x), @(window_edit_rect_min.y)) = 0) orelse (SDL_RenderCoordinatesToWindow(renderer, (edit->rect.x + edit->rect.w), (edit->rect.y + edit->rect.h), @(window_edit_rect_max.x), @(window_edit_rect_max.y)) = 0)) orelse (SDL_RenderCoordinatesToWindow(renderer, edit->cursor_rect.x, edit->cursor_rect.y, @(window_cursor.x), @(window_cursor.y)) = 0)) then
			scope
				exit sub
			end scope
		end if
		dim rect as SDL_Rect
		rect.x = cast(long, SDL_roundf(window_edit_rect_min.x))
		rect.y = cast(long, SDL_roundf(window_edit_rect_min.y))
		rect.w = cast(long, SDL_roundf((window_edit_rect_max.x - window_edit_rect_min.x)))
		rect.h = cast(long, SDL_roundf((window_edit_rect_max.y - window_edit_rect_min.y)))
		dim cursor_offset as long = cast(long, SDL_roundf((window_cursor.x - window_edit_rect_min.x)))
		SDL_SetTextInputArea(edit->window_, @(rect), cursor_offset)
	end scope
end sub

sub DrawCursor cdecl(byval edit as EditBox ptr)
	scope
		if (edit->composition_length > 0) then
			scope
				DrawCompositionCursor(edit)
				exit sub
			end scope
		end if
		dim renderer as SDL_Renderer ptr = edit->renderer
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
		SDL_RenderFillRect(renderer, @(edit->cursor_rect))
	end scope
end sub

function EditBox_Create cdecl(byval window_ as SDL_Window ptr, byval renderer as SDL_Renderer ptr, byval engine as TTF_TextEngine ptr, byval font as TTF_Font ptr, byval rect as const SDL_FRect ptr) as EditBox ptr
	scope
		dim edit as EditBox ptr = cptr(EditBox ptr, SDL_calloc(1, sizeof(((*edit)))))
		if (edit = 0) then
			scope
				return cptr(EditBox ptr, 0)
			end scope
		end if
		edit->window_ = window_
		edit->renderer = renderer
		edit->font = font
		edit->text = TTF_CreateText(engine, font, cptr(const zstring ptr, 0), 0)
		if (edit->text = 0) then
			scope
				EditBox_Destroy(edit)
				return cptr(EditBox ptr, 0)
			end scope
		end if
		edit->rect = (*rect)
		edit->highlight1 = (-1)
		edit->highlight2 = (-1)
		'' Wrap the editbox text within the editbox area
		TTF_SetTextWrapWidth(edit->text, cast(long, SDL_floorf(rect->w)))
		'' Show the whitespace when wrapping, so it can be edited
		TTF_SetTextWrapWhitespaceVisible(edit->text, true)
		'' Grab the window surface if we want to test the surface text engine.
		'' This isn't strictly necessary, we can still use the renderer if it's
		'' a software renderer targeting an SDL_Surface.
		edit->window_surface = cptr(SDL_Surface ptr, SDL_GetPointerProperty(SDL_GetRendererProperties(renderer), strptr(SDL_PROP_RENDERER_SURFACE_POINTER), (cptr(any ptr, 0))))
		'' We support rendering the composition and candidates
		SDL_SetHint(strptr(SDL_HINT_IME_IMPLEMENTED_UI), strptr("composition,candidates"))
		return edit
	end scope
end function

sub EditBox_Destroy cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		ClearCandidates(edit)
		TTF_DestroyText(edit->text)
		SDL_free(cptr(any ptr, edit))
	end scope
end sub

sub EditBox_SetFocus cdecl(byval edit as EditBox ptr, byval focus as boolean)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		if (edit->has_focus = focus) then
			scope
				exit sub
			end scope
		end if
		edit->has_focus = focus
		if edit->has_focus then
			scope
				SDL_StartTextInput(edit->window_)
			end scope
		else
			scope
				SDL_StopTextInput(edit->window_)
			end scope
		end if
	end scope
end sub

sub EditBox_Draw cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim renderer as SDL_Renderer ptr = edit->renderer
		dim x as single = edit->rect.x
		dim y as single = edit->rect.y
		'' Draw any highlight
		dim marker as long
		dim length as long
		if GetHighlightExtents(edit, @(marker), @(length)) then
			scope
				dim highlights as TTF_SubString ptr ptr = TTF_GetTextSubStringsForRange(edit->text, marker, length, cptr(long ptr, 0))
				if highlights then
					scope
						dim i as long
						SDL_SetRenderDrawColor(renderer, 238, 238, 0, 255)
						scope
							i = 0
							do while highlights[i]
								scope
									dim rect as SDL_FRect
									SDL_RectToFRect(@(highlights[i]->rect), @(rect))
									rect.x += x
									rect.y += y
									SDL_RenderFillRect(renderer, @(rect))
								end scope
								i += 1
							loop
						end scope
						SDL_free(cptr(any ptr, highlights))
					end scope
				end if
			end scope
		end if
		DrawText(edit, edit->text, x, y)
		if edit->has_focus then
			scope
				'' Draw the cursor
				dim now as Uint64 = SDL_GetTicks()
				if (((now - edit->last_cursor_change)) >= 500) then
					scope
						edit->cursor_visible = cast(boolean, (edit->cursor_visible = 0))
						edit->last_cursor_change = now
					end scope
				end if
				'' Calculate the cursor rect, used for positioning candidates
				dim cursor as TTF_SubString
				if TTF_GetTextSubString(edit->text, edit->cursor, @(cursor)) then
					scope
						dim cursor_rect as SDL_FRect
						SDL_RectToFRect(@(cursor.rect), @(cursor_rect))
						if (((cursor.flags and TTF_SUBSTRING_DIRECTION_MASK)) = TTF_DIRECTION_RTL) then
							scope
								cursor_rect.x += cursor.rect.w
							end scope
						end if
						cursor_rect.x += edit->rect.x
						cursor_rect.y += edit->rect.y
						cursor_rect.w = 1.0f
						scope
						end scope
						SDL_memcpy(cptr(any ptr, (@(edit->cursor_rect))), cptr(const any ptr, (@(cursor_rect))), sizeof(((*(@(cursor_rect))))))
						UpdateTextInputArea(edit)
					end scope
				end if
				if (edit->composition_length > 0) then
					scope
						DrawComposition(edit)
					end scope
				end if
				if edit->candidates then
					scope
						DrawCandidates(edit)
					end scope
				end if
				if edit->cursor_visible then
					scope
						DrawCursor(edit)
					end scope
				end if
			end scope
		end if
	end scope
end sub

function GetCursorTextIndex cdecl(byval x as long, byval substring as const TTF_SubString ptr) as long
	scope
		if (substring->flags and ((TTF_SUBSTRING_LINE_END or TTF_SUBSTRING_TEXT_END))) then
			scope
				return substring->offset
			end scope
		end if
		dim round_down as boolean
		if (((substring->flags and TTF_SUBSTRING_DIRECTION_MASK)) = TTF_DIRECTION_RTL) then
			scope
				round_down = cast(boolean, ((x > ((substring->rect.x + (substring->rect.w \ 2))))))
			end scope
		else
			scope
				round_down = cast(boolean, ((x < ((substring->rect.x + (substring->rect.w \ 2))))))
			end scope
		end if
		if round_down then
			scope
				'' Start the cursor before the selected text
				return substring->offset
			end scope
		else
			scope
				'' Place the cursor after the selected text
				return (substring->offset + substring->length)
			end scope
		end if
	end scope
end function

sub SetCursorPosition cdecl(byval edit as EditBox ptr, byval position as long)
	scope
		if (edit->composition_length > 0) then
			scope
				'' Don't let the cursor be moved into the composition
				if ((position >= edit->composition_start) andalso (position <= ((edit->composition_start + edit->composition_length)))) then
					scope
						exit sub
					end scope
				end if
				CancelComposition(edit)
			end scope
		end if
		edit->cursor = position
	end scope
end sub

sub MoveCursorIndex cdecl(byval edit as EditBox ptr, byval direction as long)
	scope
		dim substring as TTF_SubString
		if (direction < 0) then
			scope
				if TTF_GetTextSubString(edit->text, (edit->cursor - 1), @(substring)) then
					scope
						SetCursorPosition(edit, substring.offset)
					end scope
				end if
			end scope
		else
			scope
				if (TTF_GetTextSubString(edit->text, edit->cursor, @(substring)) andalso TTF_GetTextSubString(edit->text, (substring.offset + (iif((((substring.length) > (1))), (substring.length), (1)))), @(substring))) then
					scope
						SetCursorPosition(edit, substring.offset)
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub EditBox_MoveCursorLeft cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim substring as TTF_SubString
		if (TTF_GetTextSubString(edit->text, edit->cursor, @(substring)) andalso (((substring.flags and TTF_SUBSTRING_DIRECTION_MASK)) = TTF_DIRECTION_RTL)) then
			scope
				MoveCursorIndex(edit, 1)
			end scope
		else
			scope
				MoveCursorIndex(edit, (-1))
			end scope
		end if
	end scope
end sub

sub EditBox_MoveCursorRight cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim substring as TTF_SubString
		if (TTF_GetTextSubString(edit->text, edit->cursor, @(substring)) andalso (((substring.flags and TTF_SUBSTRING_DIRECTION_MASK)) = TTF_DIRECTION_RTL)) then
			scope
				MoveCursorIndex(edit, (-1))
			end scope
		else
			scope
				MoveCursorIndex(edit, 1)
			end scope
		end if
	end scope
end sub

sub EditBox_MoveCursorUp cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim substring as TTF_SubString
		if TTF_GetTextSubString(edit->text, edit->cursor, @(substring)) then
			scope
				dim fontHeight as long = TTF_GetFontHeight(edit->font)
				dim x as long
				dim y as long
				if (((substring.flags and TTF_SUBSTRING_DIRECTION_MASK)) = TTF_DIRECTION_RTL) then
					scope
						x = ((substring.rect.x + substring.rect.w) - 1)
					end scope
				else
					scope
						x = substring.rect.x
					end scope
				end if
				y = (substring.rect.y - (fontHeight \ 2))
				if TTF_GetTextSubStringForPoint(edit->text, x, y, @(substring)) then
					scope
						SetCursorPosition(edit, GetCursorTextIndex(x, @(substring)))
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub EditBox_MoveCursorDown cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim substring as TTF_SubString
		if TTF_GetTextSubString(edit->text, edit->cursor, @(substring)) then
			scope
				dim fontHeight as long = TTF_GetFontHeight(edit->font)
				dim x as long
				dim y as long
				if (((substring.flags and TTF_SUBSTRING_DIRECTION_MASK)) = TTF_DIRECTION_RTL) then
					scope
						x = ((substring.rect.x + substring.rect.w) - 1)
					end scope
				else
					scope
						x = substring.rect.x
					end scope
				end if
				y = ((substring.rect.y + substring.rect.h) + (fontHeight \ 2))
				if TTF_GetTextSubStringForPoint(edit->text, x, y, @(substring)) then
					scope
						SetCursorPosition(edit, GetCursorTextIndex(x, @(substring)))
					end scope
				end if
			end scope
		end if
	end scope
end sub

sub EditBox_MoveCursorBeginningOfLine cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim substring as TTF_SubString
		if (TTF_GetTextSubString(edit->text, edit->cursor, @(substring)) andalso TTF_GetTextSubStringForLine(edit->text, substring.line_index, @(substring))) then
			scope
				SetCursorPosition(edit, substring.offset)
			end scope
		end if
	end scope
end sub

sub EditBox_MoveCursorEndOfLine cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim substring as TTF_SubString
		if (TTF_GetTextSubString(edit->text, edit->cursor, @(substring)) andalso TTF_GetTextSubStringForLine(edit->text, substring.line_index, @(substring))) then
			scope
				SetCursorPosition(edit, (substring.offset + substring.length))
			end scope
		end if
	end scope
end sub

sub EditBox_MoveCursorBeginning cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		'' Move to the beginning of the text
		SetCursorPosition(edit, 0)
	end scope
end sub

sub EditBox_MoveCursorEnd cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		'' Move to the end of the text
		if edit->text->text then
			scope
				SetCursorPosition(edit, cast(long, SDL_strlen(edit->text->text)))
			end scope
		end if
	end scope
end sub

sub EditBox_Backspace cdecl(byval edit as EditBox ptr)
	scope
		if ((edit = 0) orelse (edit->text->text = 0)) then
			scope
				exit sub
			end scope
		end if
		if EditBox_DeleteHighlight(edit) then
			scope
				exit sub
			end scope
		end if
		if (edit->cursor > 0) then
			scope
				dim start as const zstring ptr = @(cptr(byte ptr, edit->text->text)[edit->cursor])
				dim next_ as const zstring ptr = start
				SDL_StepBackUTF8(edit->text->text, @(next_))
				dim length as long = cast(long, cast(uinteger, ((start - next_))))
				TTF_DeleteTextString(edit->text, (edit->cursor - length), length)
				edit->cursor -= length
			end scope
		end if
	end scope
end sub

sub EditBox_BackspaceToBeginning cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		'' Delete to the beginning of the string
		TTF_DeleteTextString(edit->text, 0, edit->cursor)
		SetCursorPosition(edit, 0)
	end scope
end sub

sub EditBox_DeleteToEnd cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		'' Delete to the end of the string
		TTF_DeleteTextString(edit->text, edit->cursor, (-1))
	end scope
end sub

sub EditBox_Delete cdecl(byval edit as EditBox ptr)
	scope
		if ((edit = 0) orelse (edit->text->text = 0)) then
			scope
				exit sub
			end scope
		end if
		if EditBox_DeleteHighlight(edit) then
			scope
				exit sub
			end scope
		end if
		dim start as const zstring ptr = @(cptr(byte ptr, edit->text->text)[edit->cursor])
		dim next_ as const zstring ptr = start
		dim length as uinteger = SDL_strlen(next_)
		SDL_StepUTF8(@(next_), @(length))
		length = ((next_ - start))
		TTF_DeleteTextString(edit->text, edit->cursor, cast(long, length))
	end scope
end sub

function HandleMouseDown cdecl(byval edit as EditBox ptr, byval x as single, byval y as single) as boolean
	scope
		dim pt as SDL_FPoint = type<SDL_FPoint>(x, y)
		if (SDL_PointInRectFloat(@(pt), @(edit->rect)) = 0) then
			scope
				if edit->has_focus then
					scope
						EditBox_SetFocus(edit, false)
						return true
					end scope
				end if
				return false
			end scope
		end if
		if (edit->has_focus = 0) then
			scope
				EditBox_SetFocus(edit, true)
			end scope
		end if
		'' Set the cursor position
		dim substring as TTF_SubString
		dim textX as long = cast(long, SDL_roundf((x - edit->rect.x)))
		dim textY as long = cast(long, SDL_roundf((y - edit->rect.y)))
		if (TTF_GetTextSubStringForPoint(edit->text, textX, textY, @(substring)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't get cursor location: %s"), SDL_GetError())
				return false
			end scope
		end if
		SetCursorPosition(edit, GetCursorTextIndex(textX, @(substring)))
		edit->highlighting = true
		edit->highlight1 = edit->cursor
		edit->highlight2 = (-1)
		return true
	end scope
end function

function HandleMouseMotion cdecl(byval edit as EditBox ptr, byval x as single, byval y as single) as boolean
	scope
		if (edit->highlighting = 0) then
			scope
				return false
			end scope
		end if
		'' Set the highlight position
		dim substring as TTF_SubString
		dim textX as long = cast(long, SDL_roundf((x - edit->rect.x)))
		dim textY as long = cast(long, SDL_roundf((y - edit->rect.y)))
		if (TTF_GetTextSubStringForPoint(edit->text, textX, textY, @(substring)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't get cursor location: %s"), SDL_GetError())
				return false
			end scope
		end if
		SetCursorPosition(edit, GetCursorTextIndex(textX, @(substring)))
		edit->highlight2 = edit->cursor
		return true
	end scope
end function

function HandleMouseUp cdecl(byval edit as EditBox ptr, byval x as single, byval y as single) as boolean
	scope
		if (edit->highlighting = 0) then
			scope
				return false
			end scope
		end if
		edit->highlighting = false
		return true
	end scope
end function

sub EditBox_SelectAll cdecl(byval edit as EditBox ptr)
	scope
		if ((edit = 0) orelse (edit->text->text = 0)) then
			scope
				exit sub
			end scope
		end if
		edit->highlight1 = 0
		edit->highlight2 = cast(long, SDL_strlen(edit->text->text))
	end scope
end sub

function EditBox_DeleteHighlight cdecl(byval edit as EditBox ptr) as boolean
	scope
		if ((edit = 0) orelse (edit->text->text = 0)) then
			scope
				return false
			end scope
		end if
		dim marker as long
		dim length as long
		if GetHighlightExtents(edit, @(marker), @(length)) then
			scope
				TTF_DeleteTextString(edit->text, marker, length)
				SetCursorPosition(edit, marker)
				edit->highlight1 = (-1)
				edit->highlight2 = (-1)
				return true
			end scope
		end if
		return false
	end scope
end function

sub EditBox_Copy cdecl(byval edit as EditBox ptr)
	scope
		if ((edit = 0) orelse (edit->text->text = 0)) then
			scope
				exit sub
			end scope
		end if
		dim marker as long
		dim length as long
		if GetHighlightExtents(edit, @(marker), @(length)) then
			scope
				dim temp as zstring ptr = cptr(zstring ptr, SDL_malloc((length + 1)))
				if temp then
					scope
						SDL_memcpy(cptr(any ptr, temp), cptr(const any ptr, @(cptr(byte ptr, edit->text->text)[marker])), length)
						cptr(byte ptr, temp)[length] = 0
						SDL_SetClipboardText(temp)
						SDL_free(cptr(any ptr, temp))
					end scope
				end if
			end scope
		else
			scope
				SDL_SetClipboardText(edit->text->text)
			end scope
		end if
	end scope
end sub

sub EditBox_Cut cdecl(byval edit as EditBox ptr)
	scope
		if ((edit = 0) orelse (edit->text->text = 0)) then
			scope
				exit sub
			end scope
		end if
		'' Copy to clipboard and delete text
		dim marker as long
		dim length as long
		if GetHighlightExtents(edit, @(marker), @(length)) then
			scope
				dim temp as zstring ptr = cptr(zstring ptr, SDL_malloc((length + 1)))
				if temp then
					scope
						SDL_memcpy(cptr(any ptr, temp), cptr(const any ptr, @(cptr(byte ptr, edit->text->text)[marker])), length)
						cptr(byte ptr, temp)[length] = 0
						SDL_SetClipboardText(temp)
						SDL_free(cptr(any ptr, temp))
					end scope
				end if
				TTF_DeleteTextString(edit->text, marker, length)
				SetCursorPosition(edit, marker)
				edit->highlight1 = (-1)
				edit->highlight2 = (-1)
			end scope
		else
			scope
				SDL_SetClipboardText(edit->text->text)
				TTF_DeleteTextString(edit->text, 0, (-1))
			end scope
		end if
	end scope
end sub

sub EditBox_Paste cdecl(byval edit as EditBox ptr)
	scope
		if (edit = 0) then
			scope
				exit sub
			end scope
		end if
		dim text as const zstring ptr = SDL_GetClipboardText()
		EditBox_Insert(edit, text)
	end scope
end sub

sub EditBox_Insert cdecl(byval edit as EditBox ptr, byval text as const zstring ptr)
	scope
		if ((edit = 0) orelse (text = 0)) then
			scope
				exit sub
			end scope
		end if
		EditBox_DeleteHighlight(edit)
		if (edit->composition_length > 0) then
			scope
				TTF_DeleteTextString(edit->text, edit->composition_start, edit->composition_length)
				edit->composition_length = 0
			end scope
		end if
		dim length as uinteger = SDL_strlen(text)
		TTF_InsertTextString(edit->text, edit->cursor, text, length)
		SetCursorPosition(edit, cast(long, ((edit->cursor + length))))
	end scope
end sub

function EditBox_HandleEvent cdecl(byval edit as EditBox ptr, byval event as SDL_Event ptr) as boolean
	scope
		if ((edit = 0) orelse (event = 0)) then
			scope
				return false
			end scope
		end if
		select case event->type
			case SDL_EVENT_MOUSE_BUTTON_DOWN
				goto switch_case_10
			case SDL_EVENT_MOUSE_MOTION
				goto switch_case_11
			case SDL_EVENT_MOUSE_BUTTON_UP
				goto switch_case_12
			case SDL_EVENT_KEY_DOWN
				goto switch_case_13
			case SDL_EVENT_TEXT_INPUT
				goto switch_case_14
			case SDL_EVENT_TEXT_EDITING
				goto switch_case_15
			case SDL_EVENT_TEXT_EDITING_CANDIDATES
				goto switch_case_16
			case else
				goto switch_case_17
		end select
		switch_case_10:
		scope
			return HandleMouseDown(edit, event->button.x, event->button.y)
		end scope
		switch_case_11:
		scope
			return HandleMouseMotion(edit, event->motion.x, event->motion.y)
		end scope
		switch_case_12:
		scope
			return HandleMouseUp(edit, event->button.x, event->button.y)
		end scope
		switch_case_13:
		scope
			if (edit->has_focus = 0) then
				scope
					goto switch_done_18
				end scope
			end if
			select case event->key.key
				case (97u)
					goto switch_case_19
				case (99u)
					goto switch_case_20
				case (118u)
					goto switch_case_21
				case (120u)
					goto switch_case_22
				case (1073741904u)
					goto switch_case_23
				case (1073741903u)
					goto switch_case_24
				case (1073741906u)
					goto switch_case_25
				case (1073741905u)
					goto switch_case_26
				case (1073741898u)
					goto switch_case_27
				case (1073741901u)
					goto switch_case_28
				case (8u)
					goto switch_case_29
				case (127u)
					goto switch_case_30
				case (13u)
					goto switch_case_31
				case (27u)
					goto switch_case_32
				case else
					goto switch_case_33
			end select
			switch_case_19:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_SelectAll(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_20:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_Copy(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_21:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_Paste(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_22:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_Cut(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_23:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_MoveCursorBeginningOfLine(edit)
					end scope
				else
					scope
						EditBox_MoveCursorLeft(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_24:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_MoveCursorEndOfLine(edit)
					end scope
				else
					scope
						EditBox_MoveCursorRight(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_25:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_MoveCursorBeginning(edit)
					end scope
				else
					scope
						EditBox_MoveCursorUp(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_26:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_MoveCursorEnd(edit)
					end scope
				else
					scope
						EditBox_MoveCursorDown(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_27:
			scope
				EditBox_MoveCursorBeginning(edit)
				goto switch_done_34
			end scope
			switch_case_28:
			scope
				EditBox_MoveCursorEnd(edit)
				goto switch_done_34
			end scope
			switch_case_29:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_BackspaceToBeginning(edit)
					end scope
				else
					scope
						EditBox_Backspace(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_30:
			scope
				if (event->key.mod_ and (SDL_KMOD_CTRL)) then
					scope
						EditBox_DeleteToEnd(edit)
					end scope
				else
					scope
						EditBox_Delete(edit)
					end scope
				end if
				goto switch_done_34
			end scope
			switch_case_31:
			scope
				EditBox_Insert(edit, strptr(!"\n"))
				goto switch_done_34
			end scope
			switch_case_32:
			scope
				EditBox_SetFocus(edit, false)
				goto switch_done_34
			end scope
			switch_case_33:
			scope
				goto switch_done_34
			end scope
			switch_done_34:
			return true
		end scope
		switch_case_14:
		scope
			EditBox_Insert(edit, event->text.text)
			return true
		end scope
		switch_case_15:
		scope
			HandleComposition(edit, @(event->edit))
			goto switch_done_18
		end scope
		switch_case_16:
		scope
			ClearCandidates(edit)
			SaveCandidates(edit, event)
			goto switch_done_18
		end scope
		switch_case_17:
		scope
			goto switch_done_18
		end scope
		switch_done_18:
		return false
	end scope
end function

'' end of editbox.bas
