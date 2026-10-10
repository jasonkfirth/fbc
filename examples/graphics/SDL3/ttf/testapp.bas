'' Project: FreeBASIC SDL3 examples
'' File: testapp.bas
'' Purpose: Port upstream SDL3_ttf-3.2.2/examples/testapp.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' testapp:  An example of using the SDL_ttf library with OpenGL.
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

'' A simple program to test the text rendering feature of the TTF library,
'' and find bugs
''
'' <F1> for console help
''
'' Use
'' - fill/replace test_fonts[] with your fonts
'' - replay a case, adding the log at line ~650 (search for 'replay')
'' - add more string to test_strings[]
''
dim shared g_force_no_SDF as long = 0
dim shared test_fonts(0 to 23) as const zstring ptr = { _
	strptr("fonts/NotoColorEmoji.ttf"), strptr("fonts/FletcherGothicFLF.ttf"), strptr("fonts/ITCAvantGardeStd-Demi.ttf"), strptr("fonts/DroidSansFallback.ttf"), strptr(!"fonts/\&o320\&o237\&o320\&o260\&o320\&o277\&o320\&o272\&o320\&o260/Lazy.ttf"), strptr("fonts/DroidSans.ttf"), strptr("fonts/DejaVuSans.ttf"), strptr("fonts/digital-7.ttf"), strptr("fonts/HelveticaNeue-Regular.ttf"), strptr("fonts/ttf_amiga/amiga4ever pro2.ttf"), strptr("fonts/ttf-bitstream-vera-1.10/VeraMoBI.ttf"), strptr("fonts/ttf-bitstream-vera-1.10/VeraBd.ttf"), strptr("fonts/ttf-bitstream-vera-1.10/Vera.ttf"), strptr("fonts/nonscalable/sazanami-gothic.ttf"), strptr("fonts/drakono.ttf"), strptr("fonts/Lazy.ttf"), _
	strptr("fonts/font_bug254.ttf"), strptr("fonts/OpenSansEmoji.ttf"), strptr("fonts/nonscalable/pvfixed_20b.pcf.gz"), strptr("fonts/nonscalable/pvfixed_20r.pcf.gz"), strptr("fonts/nonscalable/7x13B-ISO8859-15.pcf.gz"), strptr("fonts/nonscalable/vgasys.fon"), strptr("fonts/nonscalable/fixed8.fon"), strptr("fonts/Kaumudi.ttf") _
}
dim shared test_fonts_count as long = (((sizeof(const zstring ptr) * 24) \ sizeof((test_fonts(0)))))
dim shared wrap_size as long = 137
dim shared w_align as long = 0
dim shared outline as long = 0
dim shared font_style as long = 0
dim shared kerning as long = 1
dim shared wrap as long = 0
dim shared sdf as long = 0
dim shared hinting as long = 0
dim shared curr_str as long = 0
dim shared curr_font as long = 1
dim shared curr_size as long = 50
dim shared fg_alpha as long = 0
dim shared bg_alpha as long = 0
dim shared background_color as long = 0
dim shared seed as long = 0
dim shared print_elapsed_ticks as long = 0
dim shared update_screen_mode as long = 0
dim shared save_to_bmp as long = 0
'' RENDER_SOLID = 0, RENDER_BLENDED = 1, RENDER_SHADED = 2, RENDER_LCD = 3 }
dim shared render_mode as long = (-1)
dim shared render_mode_overwrite as long
dim shared render_mode_desc(0 to 3) as const zstring ptr = {strptr("Solid"), strptr("Blended"), strptr("Shaded"), strptr("LCD")}
dim shared render_mode_count as long = (((sizeof(const zstring ptr) * 4) \ sizeof((render_mode_desc(0)))))
dim shared textengine_mode as long = 0
dim shared textengine_desc(0 to 2) as const zstring ptr = {strptr("None"), strptr("Surface"), strptr("Renderer")}
dim shared textengine_count as long = (((sizeof(const zstring ptr) * 3) \ sizeof((textengine_desc(0)))))
dim shared direction as long = 0
type DirectionsRecord
	description as const zstring ptr
	value as long
end type

dim shared directions(0 to 3) as DirectionsRecord = {type<DirectionsRecord>(strptr("LTR"), TTF_DIRECTION_LTR), type<DirectionsRecord>(strptr("RTL"), TTF_DIRECTION_RTL), type<DirectionsRecord>(strptr("TTB"), TTF_DIRECTION_TTB), type<DirectionsRecord>(strptr("BTT"), TTF_DIRECTION_BTT)}
dim shared direction_count as long = (((sizeof(DirectionsRecord) * 4) \ sizeof((directions(0)))))
'' static const char *hinting_desc[] = { "normal", "light", "light_subpix", "lcd_subpix", "mono", "none" };
dim shared hinting_desc(0 to 4) as const zstring ptr = {strptr("normal"), strptr("light"), strptr("light_subpix"), strptr("mono"), strptr("none")}
dim shared hinting_count as long = (((sizeof(const zstring ptr) * 5) \ sizeof((hinting_desc(0)))))
dim shared mode_random_test as long = 0
dim shared random_cnt as long = 0
dim shared saved_curr_font as long = (-1)
dim shared saved_curr_size as long = (-1)
dim shared boardcol as SDL_Color = type<SDL_Color>(0, 0, 0, 0)
dim shared textcol as SDL_Color = type<SDL_Color>(255, 255, 255, 0)
dim shared emoji(0 to 4) as byte = {240, 159, 152, 129, 0}
dim shared test_strings(0 to 75) as const zstring ptr = { _
	strptr(!"\&o347\&o272\&o270\&o347\&o211\&o214\&o346\&o216\&o245\&o351\&o276\&o231"), strptr("ABC"), strptr(!"The quick \n brown fox \n jumps over the lazy dog 0123456789"), strptr("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"), @emoji(0), strptr((!"abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmno" & _
		!"pqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abc" & _
		!"defghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqr" & _
		!"stuvwxyz abcdefghijklmnopqrstuvwxyz ")), strptr("aTa kerning iTo"), strptr("AV VA Te eT Tr rT Td TA LV Vd pV Vq bV"), strptr("if fi ifi aif fai aifia substitution"), strptr(!"abc \&o304\&o200"), strptr((!"abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmno" & _
		!"pqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abc" & _
		!"defghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqrstuvwxyz abcdefghijklmnopqr" & _
		!"stuvwxyz abcdefghijklmnopqrstuvwxyz ")), strptr("abcdefghijklmnopqrstuvwxyz"), strptr("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), strptr("0123456789"), strptr("="), strptr("~"), _
	strptr(!"`~!@#$%^&*()_-+={}|[]\\;:'\",<.>/?"), strptr(!"\&o303\&o205"), strptr(!"A\n\b\nB"), strptr(!"Tap \&o303\&o205"), strptr("Jap"), strptr(!"A\&o315\&o241B"), strptr(!"\&o315\&o241BC"), strptr(!".\&o315\&o241BC"), strptr(!".\&o315\&o241BC\naaaaa"), strptr(!".aaaaa\n.\&o315\&o241BC"), strptr(!"aaa\n.\&o315\&o241BCaa\n.aaa\n.\&o315\&o241BCaaaaa"), strptr((!"Tap \&o303\&o205  \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205  \&o303\&o205 " & _
		!"\&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303" & _
		!"\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 " & _
		!"\&o303\&o205 \&o303\&o205Tap \&o303\&o205  \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 " & _
		!"\&o303\&o205  \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303" & _
		!"\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 " & _
		!"\&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205Tap \&o303\&o205  \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 " & _
		!"\&o303\&o205 \&o303\&o205 \&o303\&o205  \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303" & _
		!"\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 " & _
		!"\&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205 \&o303\&o205")), strptr("+"), strptr("2"), strptr(")"), strptr("|"), _
	strptr(!"a\tsdfsad.f..| I V I A AV A V D \n F G H "), strptr(!"\&o303\&o251 \&o303\&o252 \&o303\&o250 \&o303\&o240 \&o303\&o242 \&o303\&o264 \&o303\&o256 \&o303\&o257 fi if \&o303\&o261 \&o303\&o247 \&o303\&o203 \&o303\&o202"), strptr("Kl1AqvJAUOOKzdjpGTteM775hTYEuqvV b3EU0RZ88JIAMRVS7rMbyKsF1xMg9RTe"), strptr("CGOXraiYCL44XSlvqx825GvZT3HQYXNQcVNjgzrccX1eUar8nG0FZ3MsYdB5caGqiG7l7RJCyNZFRUVQ8rEVnc0TJVrY3ef4v8miQpVvYswTjjZKTZLwwFxLbsHhaDQ3"), strptr("7I8DRXPfbKUIl6SH5V6Twqw6mzEzTlDf"), strptr("JJsSGv1EA MpN9FIctQxi9D iHgEQZe2Wg"), strptr("xqKyuaT8H2xmySzWdbo wgMZ4Sp2BLUGj"), strptr("hUEKUunoouIjw6Cu jdXnPkQzMNGG7U8u"), strptr("4GDEgeCgEXwxoszBpriddAibgbhh36Vz"), strptr("FdkAi hx8B3QvmjFXovy64TzYwaZ0uoW8"), strptr("zAcp1V4PmI5dBv wtnbHqN1ISyrXB7P32"), strptr("lJq5w9GNhpqveQ67kxyQlgAwgQz1AAj3"), strptr("yi0jssXmCMjnFjl2rWs0rNIx51iVNLoI"), strptr("HT1BYxlX3z39XBFEw8JMNwYdWeQrO8h2"), strptr("aQmJI2hUruwhqvJeWL3 gZCGAZ09q67nR"), strptr("rxGWF OPfQdxUR0cYRenCccz5XgSpTDqf"), _
	strptr("4NI1L2KbwybObecSDAAgb0K00oCEkf3E"), strptr("heBlACb AfcK9G85Gl6mBpBUiblc81FSe"), strptr("GwWx6pHvWyvxSu82PbJbZI UNwx9JSATu"), strptr("AFCQd25jnbBJ7LBbaXI3I5IeSwDXqME6"), strptr("lzSqw9t7hJ03AkTjWWUyd3KqrnGAvPJg"), strptr("pxwCKuWxUOrIMvqXGL 5fbibttAPNIJFC"), strptr("Fs6sFtGnRezLttwm8AbSKsuvLGbcqFbZ"), strptr("hEdVvdwWDwsDA23dX0NjDqzYEaURRC7a"), strptr("IBrdiqo3g0tj LkXz1a45DJD52tO8FJSw"), strptr("h5QYsL9S1cfsWzTyyFJ6Npq7vLi1tjEX"), strptr("zhUkVyCPdhWspT9kBQpOmmREmSU6GPW3"), strptr("Ud1ih3stL5nq00PJMRoA1Zd1QJi0mOzj"), strptr("iVppAFr5pGYa4oT8OozDi0oqGvqJg3Eg"), strptr("YC9YUpKq95nIe7D3ckdc6Yy1cEmmuqf1"), strptr("pzmZvs5j77aVaXXnPqzcuU9fW4Kfmiv5"), strptr("04ifd9OyA5XGsLTovRo04MnIKToW9Qu3"), _
	strptr("iDJcbD5si6xPRcPRBEVeEhcJSs7n65SD"), strptr("a4oZMmgJQAa72o6sllH4n0zfJE5H0HlA"), strptr("5eSVmFkJMmPCRKdTZRJeuW71uhprmYHp"), strptr("LcnSHbxguPfqPNhdDR4XDc2qv01Q4Y1Y"), strptr("H3wT59MiQmEBZ7tYGMt1xMUu7wajECo6"), strptr("JNGUtcQz660ApgN54rk2UzpqWimO25Cy"), strptr("ss887dpFwjPaqAg53K6qZnc1NFiXc9WX"), strptr("y9Th4vRk1jSpKlzB5soK66ckzl3USZq6"), strptr("WGWY5YmWhaA5oKCoDmRA3n82XwclqvSP"), strptr("hvlSZ6jE2BU9ImjUjQiY255GA5ASfUUx"), strptr("e9o4tBCA9TCpilOI05UyHzes6s8lP9lQ"), strptr("gJyC26gZsCKR8wSp9kNMYKJRRgA3u45U1DWdzPCWv1SUEKi3Wdo3zNFTWiMcCfcl5A0MzOhbqRee7OP13NruY WP0ufGiB4W9RBWqgSy7umnE6puTyCc9WhPOzdLz168BhQwYetZkADWibObi8jcYajuUv54zxXkXQwC1B8lAi8rIH9lmIy0G10fQ832HKiLx") _
}
dim shared test_strings_count as long = (((sizeof(const zstring ptr) * 76) \ sizeof((test_strings(0)))))
dim shared font_path as const zstring ptr = cptr(const zstring ptr, 0)
dim shared font as TTF_Font ptr = cptr(TTF_Font ptr, 0)
dim shared iter as long = 0
dim shared sum as long = 0

'' The automated runner supplies one known font. Normal launches keep the
'' maintainer's original collection and font selection controls.
dim shared smoke_font as string
smoke_font = environ("FB_SDL3_TEST_FONT")
if len(smoke_font) > 0 then
	test_fonts(0) = strptr(smoke_font)
	test_fonts_count = 1
	curr_font = 0
end if

declare function testapp_basename cdecl(byval path as const zstring ptr) as zstring ptr
declare sub help cdecl()
declare function rand_n cdecl(byval n as long) as long
declare sub init_rand cdecl()
declare sub quit cdecl(byval msg as const zstring ptr)
declare sub random_input cdecl()
declare function wait_for_input cdecl() as long
declare function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long

'' make random fuzzer faster by disabling SDF rendering
'' #define HAVE_LCD
'' Need patch to set size dynamically
'' https://bugzilla.libsdl.org/show_bug.cgi?id=2487
function testapp_basename cdecl(byval path as const zstring ptr) as zstring ptr
	scope
		static buffer(0 to 1023) as byte
		dim pos_ as const zstring ptr
		dim sep as const zstring ptr
		dim prev_sep as const zstring ptr
		dim prev_was_sep as long = 0
		sep = cptr(const zstring ptr, 0)
		prev_sep = sep
		scope
			pos_ = path
			do while (*pos_)
				scope
					if (((*pos_) = 47) orelse ((*pos_) = 92)) then
						scope
							if (prev_was_sep = 0) then
								scope
									prev_sep = sep
								end scope
							end if
							sep = pos_
							prev_was_sep = 1
						end scope
					else
						scope
							prev_was_sep = 0
						end scope
					end if
				end scope
				pos_ += 1
			loop
		end scope
		if (sep = 0) then
			scope
				if (cptr(byte ptr, path)[0] = 0) then
					scope
						cptr(byte ptr, @buffer(0))[0] = 46
						cptr(byte ptr, @buffer(0))[1] = 0
					end scope
				else
					scope
						SDL_strlcpy(@buffer(0), path, (sizeof(byte) * 1024))
					end scope
				end if
			end scope
		else
			scope
				if (cptr(byte ptr, sep)[1] = 0) then
					scope
						if prev_sep then
							scope
								dim s as zstring ptr
								SDL_strlcpy(@buffer(0), (prev_sep + 1), (sizeof(byte) * 1024))
								SDL_strtok_r(@buffer(0), strptr(!"/\\"), @(s))
							end scope
						else
							scope
								cptr(byte ptr, @buffer(0))[0] = (*sep)
								cptr(byte ptr, @buffer(0))[1] = 0
							end scope
						end if
					end scope
				else
					scope
						SDL_strlcpy(@buffer(0), (sep + 1), (sizeof(byte) * 1024))
					end scope
				end if
			end scope
		end if
		return @buffer(0)
	end scope
end function

sub help cdecl()
	scope
		SDL_Log_(strptr("up/down : font size -/+"))
		SDL_Log_(strptr("c/v : next/previous font"))
		SDL_Log_(strptr("o/p : outline   -/+"))
		SDL_Log_(strptr("q/e : wrap size -/+"))
		SDL_Log_(strptr("a : align wrap: left center right"))
		SDL_Log_(strptr("w   : wrap"))
		SDL_Log_(strptr("u   : underline"))
		SDL_Log_(strptr("k   : kerning"))
		SDL_Log_(strptr("s   : strike-through"))
		SDL_Log_(strptr("b   : bold"))
		SDL_Log_(strptr("g/h : hinting -/+ (normal, light, light_subpix, mono, none)"))
		SDL_Log_(strptr("i   : italic"))
		SDL_Log_(strptr("f   : signed distance field"))
		SDL_Log_(strptr("t   : ticks elapsed for 50 rendering"))
		SDL_Log_(strptr("d   : display normal texture, no screen update, stream texture "))
		SDL_Log_(strptr("r   : start/stop random test"))
		SDL_Log_(strptr("m   : render mode Solid/Blended/Shaded"))
		SDL_Log_(strptr("x   : text engine None/Surface/Renderer"))
		SDL_Log_(strptr("n   : change direction"))
		SDL_Log_(strptr("9/0 : -/+ alpha color fg"))
		SDL_Log_(strptr("7/8 : -/+ alpha color bg (Shaded only)"))
		SDL_Log_(strptr("6   : invert bg/fg colors"))
		SDL_Log_(strptr("345 : increase r/g/b fg color"))
		SDL_Log_(strptr("2   : background color (Black->R->G->B->W)"))
		SDL_Log_(strptr("<space> : next string/rendering function"))
		SDL_Log_(strptr("<backspace> : previous string/rendering function"))
		SDL_Log_(strptr("F1   : help"))
		SDL_Log_(strptr("F2   : save current rendering to .bmp"))
	end scope
end sub

sub quit cdecl(byval msg as const zstring ptr)
	scope
		SDL_Log_(strptr("ERROR: %s"), msg)
		SDL_Log_(strptr("SDL_GetError: %s"), SDL_GetError())
		exit_(1)
	end scope
end sub

function wait_for_input cdecl() as long
	scope
		dim done as long = 0
		dim event as SDL_Event
		do
			if ((done = 0)) = 0 then exit do
			scope
				if (SDL_WaitEvent(@(event)) = 0) then
					scope
						quit(strptr("Event handling"))
					end scope
				end if
				select case event.type
					case SDL_EVENT_KEY_DOWN
						goto switch_case_3
					case SDL_EVENT_QUIT
						goto switch_case_4
					case else
						goto switch_case_5
				end select
				switch_case_3:
				scope
					if (event.key.key = 1073741882u) then
						scope
							help()
							done = 1
						end scope
					end if
					if (event.key.key = 32u) then
						scope
							iter += 1
							sum += 1
							done = 1
						end scope
					end if
					if (event.key.key = 8u) then
						scope
							iter -= 1
							sum += 1
							done = 1
						end scope
					end if
					if (event.key.key = 1073741905u) then
						scope
							curr_size -= 1
							if (curr_size < (-1)) then
								curr_size = (-1)
							end if
							done = 1
							SDL_Log_(strptr("size: %d"), cast(long, curr_size))
						end scope
					end if
					if (event.key.key = 1073741906u) then
						scope
							curr_size += 1
							done = 1
							SDL_Log_(strptr("size: %d"), cast(long, curr_size))
						end scope
					end if
					if (event.key.key = 99u) then
						scope
							curr_font -= 1
							if (curr_font < 0) then
								curr_font = 0
							end if
							done = 1
							SDL_Log_(strptr("Switch to font %s"), test_fonts(curr_font))
						end scope
					end if
					if (event.key.key = 118u) then
						scope
							curr_font += 1
							done = 1
							if (curr_font >= test_fonts_count) then
								curr_font = (test_fonts_count - 1)
							end if
							SDL_Log_(strptr("Switch to font %s"), test_fonts(curr_font))
						end scope
					end if
					if (event.key.key = 111u) then
						scope
							outline -= 1
							if (outline < (-1)) then
								outline = (-1)
							end if
							done = 1
							SDL_Log_(strptr("outline: %d"), cast(long, outline))
						end scope
					end if
					if (event.key.key = 112u) then
						scope
							outline += 1
							done = 1
							SDL_Log_(strptr("outline: %d"), cast(long, outline))
						end scope
					end if
					if (event.key.key = 113u) then
						scope
							wrap_size -= 1
							done = 1
							SDL_Log_(strptr("wrap_size: %d"), cast(long, wrap_size))
						end scope
					end if
					if (event.key.key = 97u) then
						scope
							w_align += 1
							if (w_align = 3) then
								w_align = 0
							end if
							done = 1
							SDL_Log_(strptr("wrap_align: %d"), cast(long, w_align))
						end scope
					end if
					if (event.key.key = 101u) then
						scope
							wrap_size += 1
							done = 1
							SDL_Log_(strptr("wrap_size: %d"), cast(long, wrap_size))
						end scope
					end if
					if (event.key.key = 105u) then
						scope
							dim s as long = TTF_STYLE_ITALIC
							SDL_Log_(strptr("italic %s"), iif(((font_style and s)), strptr("removed"), strptr("added")))
							font_style xor= s
							done = 1
						end scope
					end if
					if (event.key.key = 98u) then
						scope
							dim s as long = TTF_STYLE_BOLD
							SDL_Log_(strptr("bold %s"), iif(((font_style and s)), strptr("removed"), strptr("added")))
							font_style xor= s
							done = 1
						end scope
					end if
					if (event.key.key = 117u) then
						scope
							dim s as long = TTF_STYLE_UNDERLINE
							SDL_Log_(strptr("underline %s"), iif(((font_style and s)), strptr("removed"), strptr("added")))
							font_style xor= s
							done = 1
						end scope
					end if
					if (event.key.key = 115u) then
						scope
							dim s as long = TTF_STYLE_STRIKETHROUGH
							SDL_Log_(strptr("strike-through %s"), iif(((font_style and s)), strptr("removed"), strptr("added")))
							font_style xor= s
							done = 1
						end scope
					end if
					if (event.key.key = 107u) then
						scope
							done = 1
							kerning xor= 1
							if kerning then
								scope
									SDL_Log_(strptr("kerning allowed"))
								end scope
							else
								scope
									SDL_Log_(strptr("kerning removed"))
								end scope
							end if
						end scope
					end if
					if (event.key.key = 119u) then
						scope
							done = 1
							wrap xor= 1
							if wrap then
								scope
									SDL_Log_(strptr("wrap allowed"))
								end scope
							else
								scope
									SDL_Log_(strptr("wrap removed"))
								end scope
							end if
						end scope
					end if
					if (event.key.key = 102u) then
						scope
							done = 1
							sdf xor= 1
							if sdf then
								scope
									SDL_Log_(strptr("SDF allowed"))
								end scope
							else
								scope
									SDL_Log_(strptr("SDF removed"))
								end scope
							end if
						end scope
					end if
					if (event.key.key = 116u) then
						scope
							done = 1
							print_elapsed_ticks xor= 1
							if print_elapsed_ticks then
								scope
									SDL_Log_(strptr("print_elapsed_ticks displayed"))
								end scope
							else
								scope
									SDL_Log_(strptr("print_elapsed_ticks hidden"))
								end scope
							end if
						end scope
					end if
					if (event.key.key = 100u) then
						scope
							done = 1
							update_screen_mode += 1
							update_screen_mode mod= 2
							if (update_screen_mode = 0) then
								scope
									SDL_Log_(strptr("texture displayed"))
								end scope
							else
								if (update_screen_mode = 1) then
									scope
										SDL_Log_(strptr("texture not displayed"))
									end scope
								end if
							end if
						end scope
					end if
					if (event.key.key = 103u) then
						scope
							done = 1
							hinting -= 1
							if (hinting < 0) then
								hinting = 0
							end if
							SDL_Log_(strptr("hinting: %s"), hinting_desc(hinting))
						end scope
					end if
					if (event.key.key = 104u) then
						scope
							done = 1
							hinting += 1
							hinting mod= hinting_count
							SDL_Log_(strptr("hinting: %s"), hinting_desc(hinting))
						end scope
					end if
					if (event.key.key = 114u) then
						scope
							done = 1
							mode_random_test = 1
							random_cnt = 0
							SDL_Log_(strptr("start random test"))
						end scope
					end if
					if (event.key.key = 109u) then
						scope
							done = 1
							render_mode += 1
							render_mode mod= render_mode_count
							render_mode_overwrite = render_mode
							SDL_Log_(strptr("render mode: %s"), render_mode_desc(render_mode))
						end scope
					end if
					if (event.key.key = 120u) then
						scope
							done = 1
							textengine_mode += 1
							textengine_mode mod= textengine_count
							SDL_Log_(strptr("Text Engine: %s"), textengine_desc(textengine_mode))
						end scope
					end if
					if (event.key.key = 110u) then
						scope
							done = 1
							direction += 1
							direction mod= direction_count
							SDL_Log_(strptr("direction: %s"), directions(direction).description)
						end scope
					end if
					if (event.key.key = 1073741883u) then
						scope
							done = 1
							save_to_bmp = 1
						end scope
					end if
					if (event.key.key = 57u) then
						scope
							done = 1
							fg_alpha -= 1
							if (fg_alpha < 0) then
								fg_alpha = 0
							end if
							SDL_Log_(strptr("color fg alpha = %d"), cast(long, fg_alpha))
						end scope
					end if
					if (event.key.key = 48u) then
						scope
							done = 1
							fg_alpha += 1
							if (fg_alpha > 255) then
								fg_alpha = 255
							end if
							SDL_Log_(strptr("color fg alpha = %d"), cast(long, fg_alpha))
						end scope
					end if
					if (event.key.key = 55u) then
						scope
							done = 1
							bg_alpha -= 1
							if (bg_alpha < 0) then
								bg_alpha = 0
							end if
							SDL_Log_(strptr("color bg alpha = %d"), cast(long, bg_alpha))
						end scope
					end if
					if (event.key.key = 56u) then
						scope
							done = 1
							bg_alpha += 1
							if (bg_alpha > 255) then
								bg_alpha = 255
							end if
							SDL_Log_(strptr("color bg alpha = %d"), cast(long, bg_alpha))
						end scope
					end if
					if (event.key.key = 54u) then
						scope
							dim tmp as SDL_Color = textcol
							textcol = boardcol
							boardcol = tmp
							textcol.a = 0
							boardcol.a = 0
							SDL_Log_(strptr("Invert BG / FG color"))
							done = 1
						end scope
					end if
					if (event.key.key = 51u) then
						scope
							done = 1
							textcol.r += 1
							SDL_Log_(strptr("color fg: r=%d g=%d b=%d alpha=%d"), cast(long, textcol.r), cast(long, textcol.g), cast(long, textcol.b), cast(long, textcol.a))
						end scope
					end if
					if (event.key.key = 52u) then
						scope
							done = 1
							textcol.g += 1
							SDL_Log_(strptr("color fg: r=%d g=%d b=%d alpha=%d"), cast(long, textcol.r), cast(long, textcol.g), cast(long, textcol.b), cast(long, textcol.a))
						end scope
					end if
					if (event.key.key = 53u) then
						scope
							done = 1
							textcol.b += 1
							SDL_Log_(strptr("color fg: r=%d g=%d b=%d alpha=%d"), cast(long, textcol.r), cast(long, textcol.g), cast(long, textcol.b), cast(long, textcol.a))
						end scope
					end if
					if (event.key.key = 50u) then
						scope
							dim str_(0 to 5) as const zstring ptr = {strptr("Black"), strptr("Red"), strptr("Green"), strptr("Blue"), strptr("White"), strptr("Gray")}
							background_color += 1
							if (background_color = 6) then
								background_color = 0
							end if
							SDL_Log_(strptr("Background color '%s"), str_(background_color))
							done = 1
						end scope
					end if
					if (event.key.key = 27u) then
						scope
							SDL_Log_(strptr("ESC"))
							return 1
						end scope
					end if
					goto switch_done_6
				end scope
				switch_case_4:
				scope
					return 1
				end scope
				switch_case_5:
				scope
					goto switch_done_6
				end scope
				switch_done_6:
			end scope
		loop
		return 0
	end scope
end function

function example_main cdecl(byval argc as long, byval argv as zstring ptr ptr) as long
	scope
		dim window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
		dim text_texture as SDL_Texture ptr = cptr(SDL_Texture ptr, 0)
		dim text_surface as SDL_Surface ptr = cptr(SDL_Surface ptr, 0)
		dim renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
		dim text_obj as TTF_Text ptr = cptr(TTF_Text ptr, 0)
		dim engine_surface as TTF_TextEngine ptr = cptr(TTF_TextEngine ptr, 0)
		dim engine_renderer as TTF_TextEngine ptr = cptr(TTF_TextEngine ptr, 0)
		dim windoww as long = 640
		dim windowh as long = 480
		dim text as const zstring ptr
		dim filename(0 to 255) as byte
		dim infos(0 to 255) as byte
		dim replay as long = 0
		dim t1 as long = 0
		dim t2 as long = 0
		dim t_sum as long
		dim performance_start as Uint64 = 0
		dim performance_end as Uint64 = 0
		dim performance_total as Uint64
		dim T_min as Uint64
		dim count as long
		dim count_init as long
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				quit(strptr("SDL init failed"))
			end scope
		end if
		if (TTF_Init() = 0) then
			scope
				SDL_Quit()
				quit(strptr("SDL_ttf init failed"))
			end scope
		end if
		window_ = SDL_CreateWindow(strptr(""), windoww, windowh, 0)
		if (window_ = cptr(SDL_Window ptr, (cptr(any ptr, 0)))) then
			scope
				quit(strptr("SDL windowdow setup failed"))
			end scope
		end if
		'' SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
		renderer = SDL_CreateRenderer(window_, cptr(const zstring ptr, 0))
		if (renderer = cptr(SDL_Renderer ptr, (cptr(any ptr, 0)))) then
			scope
				quit(strptr("SDL renderer setup failed"))
			end scope
		end if
		'' Display help
		help()
		'' seed=1673390190; replay=1; font_style=9; kerning=1; sdf=0; wrap=1; wrap_size=94; w_align=2; outline=7; curr_size=42; render_mode=1; curr_str=75; curr_font=1997; hinting=1; fg_alpha=90; // light Blended
		if replay then
			scope
				SDL_Log_(strptr("Replay with string _%s_"), test_strings(curr_str))
			end scope
		end if
		do
			if (1) = 0 then exit do
			scope
				'' Normal mode <space>, try all combination {strings}x{render_mode} and exit
				'' This updates "render_mode" and "curr_str"
				if ((mode_random_test = 0) andalso (replay = 0)) then
					scope
						if (iter <= 0) then
							scope
								iter = 0
							end scope
						end if
						if (iter >= (render_mode_count * test_strings_count)) then
							scope
								SDL_Log_(strptr("End of rendering!"))
								exit do
							end scope
						else
							scope
								render_mode = (iter \ test_strings_count)
								curr_str = (iter - (test_strings_count * render_mode))
							end scope
						end if
					end scope
				end if
				if render_mode_overwrite then
					scope
						render_mode = render_mode_overwrite
					end scope
				end if
				textcol.a = fg_alpha
				boardcol.a = bg_alpha
				if (saved_curr_font <> curr_font) then
					scope
						if font then
							scope
								TTF_CloseFont(font)
							end scope
						end if
						saved_curr_font = curr_font
						saved_curr_size = curr_size
						font_path = test_fonts(curr_font)
						SDL_Log_(strptr("TTF_OpenFont: %s"), font_path)
						font = TTF_OpenFont(font_path, cast(single, curr_size))
						if (font = 0) then
							scope
								quit(strptr("Font load failed"))
							end scope
						end if
					end scope
				end if
				if (saved_curr_size <> curr_size) then
					scope
						TTF_SetFontSize(font, cast(single, curr_size))
						saved_curr_size = curr_size
					end scope
				end if
				TTF_SetFontDirection(font, directions(direction).value)
				scope
					dim tmp as long
					tmp = TTF_GetFontOutline(font)
					if (tmp <> outline) then
						scope
							TTF_SetFontOutline(font, outline)
						end scope
					end if
					tmp = TTF_GetFontStyle(font)
					if (tmp <> font_style) then
						scope
							TTF_SetFontStyle(font, font_style)
						end scope
					end if
					tmp = TTF_GetFontKerning(font)
					if (tmp <> kerning) then
						scope
							TTF_SetFontKerning(font, cast(boolean, kerning))
						end scope
					end if
					tmp = TTF_GetFontSDF(font)
					if (tmp <> sdf) then
						scope
							TTF_SetFontSDF(font, cast(boolean, sdf))
						end scope
					end if
					scope
						'' static const char *hinting_desc[] = { "normal", "light", "light_subpix", "lcd_subpix", "mono", "none" };
						dim h as long = 0
						if (hinting = 0) then
							h = TTF_HINTING_NORMAL
						end if
						if (hinting = 1) then
							h = TTF_HINTING_LIGHT
						end if
						if (hinting = 2) then
							h = 4
						end if
						'' if (hinting == 3) h = 5; // TTF_HINTING_LCD_SUBPIXEL;
						if (hinting = 3) then
							h = TTF_HINTING_MONO
						end if
						if (hinting = 4) then
							h = TTF_HINTING_NONE
						end if
						tmp = TTF_GetFontHinting(font)
						if (tmp <> h) then
							scope
								TTF_SetFontHinting(font, h)
							end scope
						end if
					end scope
				end scope
				'' Get some console output out in case we crash next...
				if (mode_random_test = 0) then
					scope
						dim title(0 to 1023) as byte
						SDL_snprintf(@title(0), ((sizeof(byte) * 1024) - 1), strptr("%s Sz=%d outline=%d Hinting=%s %s"), render_mode_desc(render_mode), cast(long, curr_size), cast(long, outline), hinting_desc(hinting), testapp_basename(font_path))
						if save_to_bmp then
							scope
								SDL_snprintf(@filename(0), ((sizeof(byte) * 256) - 1), strptr("Render=%s_size=%d_outline=%d_wrap=%d_wrap_size=%d_kerning=%d_sdf=%d_italic=%d_bold=%d_underline=%d_strikethrough=%d_hinting=%s_%s__%" SDL_PRIs64 ".bmp"), render_mode_desc(render_mode), cast(long, curr_size), cast(long, outline), cast(long, wrap), cast(long, wrap_size), cast(long, kerning), cast(long, sdf), cast(long, ((((font_style and TTF_STYLE_ITALIC)) = 0) = 0)), cast(long, ((((font_style and TTF_STYLE_BOLD)) = 0) = 0)), cast(long, ((((font_style and TTF_STYLE_UNDERLINE)) = 0) = 0)), cast(long, ((((font_style and TTF_STYLE_STRIKETHROUGH)) = 0) = 0)), hinting_desc(hinting), testapp_basename(font_path), cast(Sint64, cast(Sint64, time_(cptr(time_t ptr, 0)))))
							end scope
						end if
						SDL_snprintf(@infos(0), ((sizeof(byte) * 256) - 1), strptr("Render=%s size=%d outline=%d wrap=%d wrap_size=%d kerning=%d sdf=%d italic=%d bold=%d underline=%d strikethrough=%d hinting=%s %s"), render_mode_desc(render_mode), cast(long, curr_size), cast(long, outline), cast(long, wrap), cast(long, wrap_size), cast(long, kerning), cast(long, sdf), cast(long, ((((font_style and TTF_STYLE_ITALIC)) = 0) = 0)), cast(long, ((((font_style and TTF_STYLE_BOLD)) = 0) = 0)), cast(long, ((((font_style and TTF_STYLE_UNDERLINE)) = 0) = 0)), cast(long, ((((font_style and TTF_STYLE_STRIKETHROUGH)) = 0) = 0)), hinting_desc(hinting), font_path)
						SDL_SetWindowTitle(window_, @title(0))
						SDL_Log_(strptr("%s"), @infos(0))
					end scope
				end if
				'' pick a string
				text = test_strings(curr_str)
				'' Skip some string with no chars (all index == 0), zero advance and so zero width at low size
				'' That would fail to render because of size 0.
				''
				scope
					dim w as long = 0
					dim h as long = 0
					'' SDF change the str size. if we dont render with it (eg not blended mode).
					'' then set it to 0.
					'' otherwse it reports a valid size.
					'' but the texture isn't renderered.
					if (render_mode <> 1) then
						scope
							dim tmp as long = TTF_GetFontSDF(font)
							if (tmp <> 0) then
								scope
									TTF_SetFontSDF(font, false)
								end scope
							end if
						end scope
					end if
					if (TTF_GetStringSize(font, text, 0, @(w), @(h)) = 0) then
						scope
							SDL_Log_(strptr("size failed"))
						end scope
					end if
					if (w = 0) then
						scope
							SDL_Log_(strptr("skip size == 0"))
							goto next_loop
						end scope
					end if
				end scope
				if (textengine_mode <> 0) then
					scope
						if (engine_surface = 0) then
							scope
								engine_surface = TTF_CreateSurfaceTextEngine()
								if (engine_surface = 0) then
									scope
										SDL_Log_(strptr("Couldn't create surface text engine: %s"), SDL_GetError())
									end scope
								end if
							end scope
						end if
						if (engine_renderer = 0) then
							scope
								engine_renderer = TTF_CreateRendererTextEngine(renderer)
								if (engine_renderer = 0) then
									scope
										SDL_Log_(strptr("Couldn't create renderer text engine: %s"), SDL_GetError())
									end scope
								end if
							end scope
						end if
					end scope
				end if
				count_init = iif(print_elapsed_ticks, 500, 1)
				t_sum = 0
				performance_total = 0
				T_min = 999999999999ull
				count = count_init
				'' render
				if (wrap = 0) then
					scope
						do
							dim expression_value_9 as long = count
							count -= 1
							if (expression_value_9) = 0 then exit do
							scope
								if text_surface then
									scope
										SDL_DestroySurface(text_surface)
										text_surface = cptr(SDL_Surface ptr, 0)
									end scope
								end if
								if text_obj then
									scope
										TTF_DestroyText(text_obj)
										text_obj = cptr(TTF_Text ptr, 0)
									end scope
								end if
								if (textengine_mode = 0) then
									scope
										select case render_mode
											case (0)
												goto switch_case_10
											case (1)
												goto switch_case_11
											case (2)
												goto switch_case_12
											case (3)
												goto switch_case_13
											case else
												goto switch_done_14
										end select
										switch_case_10:
										scope
											text_surface = TTF_RenderText_Solid(font, text, 0, textcol)
											goto switch_done_14
										end scope
										switch_case_11:
										scope
											text_surface = TTF_RenderText_Blended(font, text, 0, textcol)
											goto switch_done_14
										end scope
										switch_case_12:
										scope
											text_surface = TTF_RenderText_Shaded(font, text, 0, textcol, boardcol)
											goto switch_done_14
										end scope
										switch_case_13:
										scope
											text_surface = TTF_RenderText_Shaded(font, text, 0, textcol, boardcol)
											goto switch_done_14
										end scope
										switch_done_14:
									end scope
								else
									if (textengine_mode = 1) then
										scope
											text_obj = TTF_CreateText(engine_surface, font, text, 0)
										end scope
									else
										scope
											text_obj = TTF_CreateText(engine_renderer, font, text, 0)
										end scope
									end if
								end if
								if text_surface then
									scope
										SDL_DestroySurface(text_surface)
										text_surface = cptr(SDL_Surface ptr, 0)
									end scope
								end if
								if text_obj then
									scope
										TTF_DestroyText(text_obj)
										text_obj = cptr(TTF_Text ptr, 0)
									end scope
								end if
								if print_elapsed_ticks then
									scope
										t1 = SDL_GetTicks()
										performance_start = SDL_GetPerformanceCounter()
									end scope
								end if
								if (textengine_mode = 0) then
									scope
										select case render_mode
											case (0)
												goto switch_case_15
											case (1)
												goto switch_case_16
											case (2)
												goto switch_case_17
											case (3)
												goto switch_case_18
											case else
												goto switch_done_19
										end select
										switch_case_15:
										scope
											text_surface = TTF_RenderText_Solid(font, text, 0, textcol)
											goto switch_done_19
										end scope
										switch_case_16:
										scope
											text_surface = TTF_RenderText_Blended(font, text, 0, textcol)
											goto switch_done_19
										end scope
										switch_case_17:
										scope
											text_surface = TTF_RenderText_Shaded(font, text, 0, textcol, boardcol)
											goto switch_done_19
										end scope
										switch_case_18:
										scope
											text_surface = TTF_RenderText_Shaded(font, text, 0, textcol, boardcol)
											goto switch_done_19
										end scope
										switch_done_19:
									end scope
								else
									if (textengine_mode = 1) then
										scope
											text_obj = TTF_CreateText(engine_surface, font, text, 0)
										end scope
									else
										scope
											text_obj = TTF_CreateText(engine_renderer, font, text, 0)
										end scope
									end if
								end if
								if print_elapsed_ticks then
									scope
										performance_end = SDL_GetPerformanceCounter()
										t2 = SDL_GetTicks()
										t_sum += ((t2 - t1))
										performance_total += ((performance_end - performance_start))
										T_min = (iif((((T_min) < ((performance_end - performance_start)))), (T_min), ((performance_end - performance_start))))
									end scope
								end if
							end scope
						loop
					end scope
				else
					scope
						TTF_SetFontWrapAlignment(font, w_align)
						do
							dim expression_value_21 as long = count
							count -= 1
							if (expression_value_21) = 0 then exit do
							scope
								if text_surface then
									scope
										SDL_DestroySurface(text_surface)
										text_surface = cptr(SDL_Surface ptr, 0)
									end scope
								end if
								if text_obj then
									scope
										TTF_DestroyText(text_obj)
										text_obj = cptr(TTF_Text ptr, 0)
									end scope
								end if
								if print_elapsed_ticks then
									scope
										t1 = SDL_GetTicks()
										performance_start = SDL_GetPerformanceCounter()
									end scope
								end if
								if (textengine_mode = 0) then
									scope
										select case render_mode
											case (0)
												goto switch_case_22
											case (1)
												goto switch_case_23
											case (2)
												goto switch_case_24
											case (3)
												goto switch_case_25
											case else
												goto switch_done_26
										end select
										switch_case_22:
										scope
											text_surface = TTF_RenderText_Solid_Wrapped(font, text, 0, textcol, wrap_size)
											goto switch_done_26
										end scope
										switch_case_23:
										scope
											text_surface = TTF_RenderText_Blended_Wrapped(font, text, 0, textcol, wrap_size)
											goto switch_done_26
										end scope
										switch_case_24:
										scope
											text_surface = TTF_RenderText_Shaded_Wrapped(font, text, 0, textcol, boardcol, wrap_size)
											goto switch_done_26
										end scope
										switch_case_25:
										scope
											text_surface = TTF_RenderText_Shaded_Wrapped(font, text, 0, textcol, boardcol, wrap_size)
											goto switch_done_26
										end scope
										switch_done_26:
									end scope
								else
									if (textengine_mode = 1) then
										scope
											text_obj = TTF_CreateText(engine_surface, font, text, 0)
											TTF_SetTextWrapWidth(text_obj, wrap_size)
										end scope
									else
										scope
											text_obj = TTF_CreateText(engine_renderer, font, text, 0)
											TTF_SetTextWrapWidth(text_obj, wrap_size)
										end scope
									end if
								end if
								if print_elapsed_ticks then
									scope
										performance_end = SDL_GetPerformanceCounter()
										t2 = SDL_GetTicks()
										t_sum += ((t2 - t1))
										performance_total += ((performance_end - performance_start))
										T_min = (iif((((T_min) < ((performance_end - performance_start)))), (T_min), ((performance_end - performance_start))))
									end scope
								end if
							end scope
						loop
					end scope
				end if
				if print_elapsed_ticks then
					scope
						SDL_Log_(strptr("Avg: %7lg ms  Avg Perf: %7lu (min=%7lu)"), cast(double, (cast(double, t_sum) / cast(double, count_init))), cast(Uint64, (performance_total / count_init)), cast(Uint64, T_min))
					end scope
				end if
				if (textengine_mode = 0) then
					scope
						if (((text_surface = cptr(SDL_Surface ptr, (cptr(any ptr, 0)))) andalso (render_mode = 1)) andalso (sdf = 1)) then
							scope
								'' sometimes SDF has glyph not found ?? FT issue ?
								SDL_Log_(strptr("BLENDED/SDF not rendered--> %s"), SDL_GetError())
								goto next_loop
							end scope
						else
							if ((text_surface = cptr(SDL_Surface ptr, (cptr(any ptr, 0)))) andalso (render_mode = 3)) then
								scope
									SDL_Log_(strptr("LCD not rendered--> %s"), SDL_GetError())
									goto next_loop
								end scope
							else
								if (text_surface = cptr(SDL_Surface ptr, (cptr(any ptr, 0)))) then
									scope
										goto finish
									end scope
								end if
							end if
						end if
					end scope
				else
					scope
						if (text_obj = cptr(TTF_Text ptr, (cptr(any ptr, 0)))) then
							scope
								goto finish
							end scope
						end if
					end scope
				end if
				'' update_screen
				if save_to_bmp then
					scope
						save_to_bmp = 0
						SDL_Log_(strptr("Saving to %s"), @filename(0))
						SDL_SaveBMP(text_surface, @filename(0))
					end scope
				end if
				if (update_screen_mode = 1) then
					scope
					end scope
				else
					if (update_screen_mode = 0) then
						scope
							'' Normal texture
							if (background_color = 0) then
								SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255)
							end if
							if (background_color = 1) then
								SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
							end if
							if (background_color = 2) then
								SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255)
							end if
							if (background_color = 3) then
								SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255)
							end if
							if (background_color = 4) then
								SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
							end if
							if (background_color = 5) then
								SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255)
							end if
							SDL_RenderClear(renderer)
							if (textengine_mode = 0) then
								scope
									text_texture = SDL_CreateTextureFromSurface(renderer, text_surface)
									if (text_texture = cptr(SDL_Texture ptr, (cptr(any ptr, 0)))) then
										scope
											SDL_Log_(strptr("Cannot create texture from surface(w=%d h=%d): %s"), cast(long, text_surface->w), cast(long, text_surface->h), SDL_GetError())
										end scope
									else
										scope
											dim dstrect as SDL_Rect
											dstrect.x = ((windoww \ 2) - (text_surface->w \ 2))
											dstrect.y = ((windowh \ 2) - (text_surface->h \ 2))
											dstrect.w = text_surface->w
											dstrect.h = text_surface->h
											scope
												dim d as SDL_FRect
												d.x = cast(single, dstrect.x)
												d.y = cast(single, dstrect.y)
												d.w = cast(single, dstrect.w)
												d.h = cast(single, dstrect.h)
												SDL_RenderTexture(renderer, text_texture, cptr(const SDL_FRect ptr, 0), @(d))
											end scope
											SDL_RenderPresent(renderer)
											SDL3_ExampleSmokeFrame()
											SDL_DestroyTexture(text_texture)
										end scope
									end if
								end scope
							else
								if (textengine_mode = 1) then
									scope
										dim window_surface as SDL_Surface ptr = SDL_GetWindowSurface(window_)
										dim w as long
										dim h as long
										dim x as long
										dim y as long
										TTF_GetTextSize(text_obj, @(w), @(h))
										x = ((windoww \ 2) - (w \ 2))
										y = ((windowh \ 2) - (h \ 2))
										SDL_ClearSurface(window_surface, 0, 0, 0, 0)
										TTF_DrawSurfaceText(text_obj, x, y, window_surface)
										SDL_UpdateWindowSurface(window_)
									end scope
								else
									scope
										dim w as long
										dim h as long
										dim x as single
										dim y as single
										TTF_GetTextSize(text_obj, @(w), @(h))
										x = ((windoww / 2.0f) - (w / 2.0f))
										y = ((windowh / 2.0f) - (h / 2.0f))
										TTF_DrawRendererText(text_obj, x, y)
										SDL_RenderPresent(renderer)
										SDL3_ExampleSmokeFrame()
									end scope
								end if
							end if
						end scope
					end if
				end if
				if text_surface then
					scope
						SDL_DestroySurface(text_surface)
						text_surface = cptr(SDL_Surface ptr, 0)
					end scope
				end if
				if text_obj then
					scope
						TTF_DestroyText(text_obj)
						text_obj = cptr(TTF_Text ptr, 0)
					end scope
				end if
				next_loop:
				if (mode_random_test = 0) then
					scope
						if (wait_for_input() > 0) then
							scope
								exit do
							end scope
						end if
					end scope
				else
					scope
						random_input()
					end scope
				end if
			end scope
		loop
		finish:
		SDL_Log_(strptr("SDL_GetError: %s"), SDL_GetError())
		SDL_Log_(strptr("cleanup"))
		if font then
			scope
				TTF_CloseFont(font)
			end scope
		end if
		if engine_surface then
			scope
				TTF_DestroySurfaceTextEngine(engine_surface)
			end scope
		end if
		if engine_renderer then
			scope
				TTF_DestroyRendererTextEngine(engine_renderer)
			end scope
		end if
		SDL_DestroyRenderer(renderer)
		SDL_DestroyWindow(window_)
		SDL_Log_(strptr("SDL_GetError: %s"), SDL_GetError())
		TTF_Quit()
		SDL_Quit()
		SDL_Log_(strptr("SDL_GetError: %s"), SDL_GetError())
		return 0
	end scope
end function

'' random in [0; n - 1]
function rand_n cdecl(byval n as long) as long
	scope
		return ((rand() mod n))
	end scope
end function

sub init_rand cdecl()
	scope
		static once_ as long = 0
		if (once_ = 0) then
			scope
				once_ = 1
				'' Let's display the seed if we need to play the test again
				if (seed = 0) then
					scope
						seed = time_(cptr(time_t ptr, 0))
						SDL_Log_(strptr("seed %d"), cast(long, seed))
					end scope
				else
					scope
						SDL_Log_(strptr("re-use seed %d"), cast(long, seed))
					end scope
				end if
				srand(seed)
			end scope
		end if
	end scope
end sub

sub random_input cdecl()
	scope
		dim event as SDL_Event
		dim r as long
		dim r0 as long
		dim r1 as long
		dim r2 as long
		dim r3 as long
		dim r4 as long
		dim r5 as long
		dim r6 as long
		dim r7 as long
		dim r8 as long
		dim r9 as long
		dim r10 as long
		dim r11 as long
		dim r12 as long
		dim r13 as long
		dim r14 as long
		dim r15 as long
		dim r16 as long
		dim r17 as long
		dim r18 as long
		dim r19 as long
		dim r20 as long
		init_rand()
		do
			if not SDL_PollEvent(@event) then exit do
			scope
				if (event.type = SDL_EVENT_KEY_DOWN) then
					scope
						mode_random_test = 0
						SDL_Log_(strptr("stop random test"))
						exit sub
					end scope
				end if
			end scope
		loop
		random_cnt += 1
		font_style = 0
		r = rand()
		r0 = ((((r and ((1 shl 0)))) = 0) = 0)
		r1 = ((((r and ((1 shl 1)))) = 0) = 0)
		r2 = ((((r and ((1 shl 2)))) = 0) = 0)
		r3 = ((((r and ((1 shl 3)))) = 0) = 0)
		r4 = ((((r and ((1 shl 4)))) = 0) = 0)
		r5 = ((((r and ((1 shl 5)))) = 0) = 0)
		r6 = ((((r and ((1 shl 6)))) = 0) = 0)
		r7 = ((((r and ((1 shl 7)))) = 0) = 0)
		r8 = ((((r and ((1 shl 8)))) = 0) = 0)
		r9 = ((((r and ((1 shl 9)))) = 0) = 0)
		r10 = ((((r and ((1 shl 10)))) = 0) = 0)
		r11 = ((((r and ((1 shl 11)))) = 0) = 0)
		r12 = ((((r and ((1 shl 12)))) = 0) = 0)
		r13 = ((((r and ((1 shl 13)))) = 0) = 0)
		r14 = ((((r and ((1 shl 14)))) = 0) = 0)
		r15 = ((((r and ((1 shl 15)))) = 0) = 0)
		r16 = ((((r and ((1 shl 16)))) = 0) = 0)
		r17 = ((((r and ((1 shl 17)))) = 0) = 0)
		r18 = ((((r and ((1 shl 18)))) = 0) = 0)
		r19 = ((((r and ((1 shl 19)))) = 0) = 0)
		r20 = ((((r and ((1 shl 20)))) = 0) = 0)
		if r0 then
			font_style or= TTF_STYLE_UNDERLINE
		end if
		if r1 then
			font_style or= TTF_STYLE_STRIKETHROUGH
		end if
		if r2 then
			font_style or= TTF_STYLE_BOLD
		end if
		if r3 then
			font_style or= TTF_STYLE_ITALIC
		end if
		kerning = r4
		wrap = r5
		'' wrap size: change less often, arbitrary distribution
		if wrap then
			scope
				if r6 then
					scope
						wrap_size = rand_n(200)
					end scope
				else
					if r7 then
						scope
							wrap_size = rand_n(400)
						end scope
					else
						if r8 then
							scope
								wrap_size = rand_n(800)
							end scope
						else
							scope
								wrap_size = rand_n(2000)
							end scope
						end if
					end if
				end if
				w_align = rand_n(3)
			end scope
		end if
		'' Current Outline: change less often, arbitrary distribution
		if r9 then
			scope
				outline = 0
			end scope
		else
			scope
				if r10 then
					scope
						outline = rand_n(10)
					end scope
				else
					scope
						outline = rand_n(150)
					end scope
				end if
			end scope
		end if
		'' Current Size: change less often, arbitrary distribution
		if ((r11 andalso r12) andalso r13) then
			scope
				if r14 then
					scope
						curr_size = rand_n(14)
					end scope
				else
					scope
						if r15 then
							scope
								curr_size = rand_n(30)
							end scope
						else
							if r16 then
								scope
									curr_size = rand_n(60)
								end scope
							else
								scope
									curr_size = rand_n(200)
								end scope
							end if
						end if
					end scope
				end if
				curr_size += 1
			end scope
		end if
		'' Current Font: change less often
		if ((r17 andalso r18) andalso r19) then
			scope
				curr_font = rand_n(test_fonts_count)
			end scope
		end if
		sdf = r20
		'' using SDF is too slow
		if g_force_no_SDF then
			scope
				sdf = 0
			end scope
		else
			if sdf then
				scope
					'' also slow ?
					if (outline >= 4) then
						scope
							outline = 4
						end scope
					end if
				end scope
			end if
		end if
		render_mode = rand_n(render_mode_count)
		hinting = rand_n(hinting_count)
		curr_str = rand_n(test_strings_count)
		fg_alpha = rand_n(256)
		SDL_Log_(strptr("%6d] seed=%d; replay=1; font_style=%d; kerning=%d; sdf=%d; wrap=%d; wrap_size=%d; w_align=%d; outline=%d; curr_size=%d; render_mode=%d; curr_str=%d; curr_font=%d; hinting=%d; fg_alpha=%d; // %s %s"), cast(long, random_cnt), cast(long, seed), cast(long, font_style), cast(long, kerning), cast(long, sdf), cast(long, wrap), cast(long, wrap_size), cast(long, w_align), cast(long, outline), cast(long, curr_size), cast(long, render_mode), cast(long, curr_str), cast(long, curr_font), cast(long, hinting), cast(long, fg_alpha), hinting_desc(hinting), render_mode_desc(render_mode))
		exit sub
	end scope
end sub

end SDL_RunApp(__FB_ARGC__, __FB_ARGV__, @example_main, 0)

'' end of testapp.bas
