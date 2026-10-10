'' Project: FreeBASIC SDL3 examples
'' File: infinite-monkeys.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/demo/03-infinite-monkeys/infinite-monkeys.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define MIN_MONKEY_SCANCODE SDL_SCANCODE_A
#define MAX_MONKEY_SCANCODE SDL_SCANCODE_SLASH

'' use the callbacks instead of main()
'' We will use this renderer to draw into this window every frame.
dim shared window_ as SDL_Window ptr = cptr(SDL_Window ptr, 0)
dim shared renderer as SDL_Renderer ptr = cptr(SDL_Renderer ptr, 0)
dim shared text as zstring ptr
dim shared end_ as const zstring ptr
dim shared progress as const zstring ptr
dim shared start_time as SDL_Time
dim shared end_time as SDL_Time
type Line_
	text as Uint32 ptr
	length as long
end type

dim shared row as long = 0
dim shared rows as long = 0
dim shared cols as long = 0
dim shared lines as Line_ ptr ptr
dim shared monkey_chars as Line_
dim shared monkeys as long = 100
'' The highest and lowest scancodes a monkey can hit
dim shared default_text as const zstring ptr = strptr((!"Jabberwocky, by Lewis Carroll\n\n'Twas brillig, and the slithy toves\n      Did gyre and gimble in the wabe:\nAll mimsy were the borogoves,\n      And" & _
		!" the mome raths outgrabe.\n\n\"Beware the Jabberwock, my son!\n      The jaws that bite, the claws that catch!\nBeware the Jubjub bird, and shun\n    " & _
		!"  The frumious Bandersnatch!\"\n\nHe took his vorpal sword in hand;\n      Long time the manxome foe he sought-\nSo rested he by the Tumtum tree\n    " & _
		!"  And stood awhile in thought.\n\nAnd, as in uffish thought he stood,\n      The Jabberwock, with eyes of flame,\nCame whiffling through the tulgey wo" & _
		!"od,\n      And burbled as it came!\n\nOne, two! One, two! And through and through\n      The vorpal blade went snicker-snack!\nHe left it dead, and wi" & _
		!"th its head\n      He went galumphing back.\n\n\"And hast thou slain the Jabberwock?\n      Come to my arms, my beamish boy!\nO frabjous day! Callooh!" & _
		!" Callay!\"\n      He chortled in his joy.\n\n'Twas brillig, and the slithy toves\n      Did gyre and gimble in the wabe:\nAll mimsy were the borogoves" & _
		!",\n      And the mome raths outgrabe.\n"))

declare sub FreeLines cdecl()
declare sub OnWindowSizeChanged cdecl()
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare sub DisplayLine cdecl(byval x as single, byval y as single, byval line_ as Line_ ptr)
declare function CanMonkeyType cdecl(byval ch as Uint32) as boolean
declare sub AdvanceRow cdecl()
declare sub AddMonkeyChar cdecl(byval monkey as long, byval ch as Uint32)
declare function GetNextChar cdecl() as Uint32
declare function MonkeyPlay cdecl() as Uint32
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

sub FreeLines cdecl()
	scope
		dim i as long
		if ((rows > 0) andalso (cols > 0)) then
			scope
				scope
					i = 0
					do while (i < rows)
						scope
							SDL_free(cptr(any ptr, lines[i]->text))
							SDL_free(cptr(any ptr, lines[i]))
						end scope
						i += 1
					loop
				end scope
				SDL_free(cptr(any ptr, lines))
				lines = cptr(Line_ ptr ptr, 0)
			end scope
		end if
		SDL_free(cptr(any ptr, monkey_chars.text))
		monkey_chars.text = cptr(Uint32 ptr, 0)
	end scope
end sub

sub OnWindowSizeChanged cdecl()
	scope
		dim w as long
		dim h as long
		if (SDL_GetCurrentRenderOutputSize(renderer, @(w), @(h)) = 0) then
			scope
				exit sub
			end scope
		end if
		FreeLines()
		row = 0
		rows = (((h \ SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)) - 4)
		cols = ((w \ SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE))
		if ((rows > 0) andalso (cols > 0)) then
			scope
				dim i as long
				lines = cptr(Line_ ptr ptr, SDL_malloc((rows * sizeof(Line_ ptr))))
				if lines then
					scope
						scope
							i = 0
							do while (i < rows)
								scope
									lines[i] = cptr(Line_ ptr, SDL_malloc(sizeof(Line_)))
									if (lines[i] = 0) then
										scope
											FreeLines()
											exit do
										end scope
									end if
									lines[i]->text = cptr(Uint32 ptr, SDL_malloc((cols * sizeof(Uint32))))
									if (lines[i]->text = 0) then
										scope
											FreeLines()
											exit do
										end scope
									end if
									lines[i]->length = 0
								end scope
								i += 1
							loop
						end scope
					end scope
				end if
				monkey_chars.text = cptr(Uint32 ptr, SDL_malloc((cols * sizeof(Uint32))))
				if monkey_chars.text then
					scope
						scope
							i = 0
							do while (i < cols)
								scope
									monkey_chars.text[i] = 32
								end scope
								i += 1
							loop
						end scope
						monkey_chars.length = cols
					end scope
				end if
			end scope
		end if
	end scope
end sub

'' This function runs once at startup.
function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim arg as long = 1
		SDL_SetAppMetadata(strptr("Infinite Monkeys"), strptr("1.0"), strptr("com.example.infinite-monkeys"))
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/demo/infinite-monkeys"), 640, 480, 0, @(window_), @(renderer)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't create window/renderer: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderVSync(renderer, 1)
		if (argv[arg] andalso (SDL_strcmp(argv[arg], strptr("--monkeys")) = 0)) then
			scope
				arg += 1
				if argv[arg] then
					scope
						monkeys = SDL_atoi(argv[arg])
						arg += 1
					end scope
				else
					scope
						SDL_Log_(strptr("Usage: %s [--monkeys N] [file.txt]"), argv[0])
						return SDL_APP_FAILURE
					end scope
				end if
			end scope
		end if
		if argv[arg] then
			scope
				dim file_ as const zstring ptr = argv[arg]
				dim size as uinteger
				text = cptr(zstring ptr, SDL_LoadFile(file_, @(size)))
				if (text = 0) then
					scope
						SDL_Log_(strptr("Couldn't open %s: %s"), file_, SDL_GetError())
						return SDL_APP_FAILURE
					end scope
				end if
				end_ = (text + size)
			end scope
		else
			scope
				text = SDL_strdup(default_text)
				end_ = (text + SDL_strlen(text))
			end scope
		end if
		progress = text
		SDL_GetCurrentTime(@(start_time))
		OnWindowSizeChanged()
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs when a new event (mouse input, keypresses, etc) occurs.
function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		select case event->type
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED
				goto switch_case_4
			case SDL_EVENT_QUIT
				goto switch_case_5
			case else
				goto switch_done_6
		end select
		switch_case_4:
		scope
			OnWindowSizeChanged()
			goto switch_done_6
		end scope
		switch_case_5:
		scope
			return SDL_APP_SUCCESS
		end scope
		switch_done_6:
		return SDL_APP_CONTINUE
	end scope
end function

sub DisplayLine cdecl(byval x as single, byval y as single, byval line_ as Line_ ptr)
	scope
		'' Allocate maximum space potentially needed for this line
		dim utf8 as zstring ptr = cptr(zstring ptr, SDL_malloc(((line_->length * 4) + 1)))
		if utf8 then
			scope
				dim spot as zstring ptr = utf8
				dim i as long
				scope
					i = 0
					do while (i < line_->length)
						scope
							spot = SDL_UCS4ToUTF8(line_->text[i], spot)
						end scope
						i += 1
					loop
				end scope
				(*cptr(byte ptr, spot)) = 0
				SDL_RenderDebugText(renderer, x, y, utf8)
				SDL_free(cptr(any ptr, utf8))
			end scope
		end if
	end scope
end sub

function CanMonkeyType cdecl(byval ch as Uint32) as boolean
	scope
		dim modstate as SDL_Keymod
		dim scancode as SDL_Scancode = SDL_GetScancodeFromKey(ch, @(modstate))
		if ((scancode < SDL_SCANCODE_A) orelse (scancode > SDL_SCANCODE_SLASH)) then
			scope
				return false
			end scope
		end if
		'' Monkeys can hit the shift key, but nothing else
		if (((modstate and (not (SDL_KMOD_SHIFT)))) <> 0) then
			scope
				return false
			end scope
		end if
		return true
	end scope
end function

sub AdvanceRow cdecl()
	scope
		dim line_ as Line_ ptr
		row += 1
		line_ = lines[(row mod rows)]
		line_->length = 0
	end scope
end sub

sub AddMonkeyChar cdecl(byval monkey as long, byval ch as Uint32)
	scope
		if ((monkey >= 0) andalso monkey_chars.text) then
			scope
				monkey_chars.text[((monkey mod cols))] = ch
			end scope
		end if
		if lines then
			scope
				if (ch = 10) then
					scope
						AdvanceRow()
					end scope
				else
					scope
						dim line_ as Line_ ptr = lines[(row mod rows)]
						dim expression_value_8 as long = line_->length
						line_->length += 1
						line_->text[expression_value_8] = ch
						if (line_->length = cols) then
							scope
								AdvanceRow()
							end scope
						end if
					end scope
				end if
			end scope
		end if
		SDL_StepUTF8(@(progress), cptr(uinteger ptr, 0))
	end scope
end sub

function GetNextChar cdecl() as Uint32
	scope
		dim ch as Uint32 = 0
		do
			if ((progress < end_)) = 0 then exit do
			scope
				dim spot as const zstring ptr = progress
				ch = SDL_StepUTF8(@(spot), cptr(uinteger ptr, 0))
				if CanMonkeyType(ch) then
					scope
						exit do
					end scope
				else
					scope
						'' This is a freebie, monkeys can't type this
						AddMonkeyChar((-1), ch)
					end scope
				end if
			end scope
		loop
		return ch
	end scope
end function

function MonkeyPlay cdecl() as Uint32
	scope
		dim count as long = (((SDL_SCANCODE_SLASH - SDL_SCANCODE_A) + 1))
		dim scancode as SDL_Scancode = cast(SDL_Scancode, ((SDL_SCANCODE_A + SDL_rand(count))))
		dim modstate as SDL_Keymod = (iif(SDL_rand(2), (SDL_KMOD_SHIFT), 0))
		return SDL_GetKeyFromScancode(scancode, modstate, false)
	end scope
end function

'' This function runs once per frame, and is the heart of the program.
function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim i as long
		dim monkey as long
		dim next_char as Uint32 = 0
		dim ch as Uint32
		dim x as single
		dim y as single
		dim caption as zstring ptr = cptr(zstring ptr, 0)
		dim now as SDL_Time
		dim elapsed as SDL_Time
		dim hours as long
		dim minutes as long
		dim seconds as long
		dim rect as SDL_FRect
		scope
			monkey = 0
			do while (monkey < monkeys)
				scope
					if (next_char = 0) then
						scope
							next_char = GetNextChar()
							if (next_char = 0) then
								scope
									'' All done!
									exit do
								end scope
							end if
						end scope
					end if
					ch = MonkeyPlay()
					if (ch = next_char) then
						scope
							AddMonkeyChar(monkey, ch)
							next_char = 0
						end scope
					end if
				end scope
				monkey += 1
			loop
		end scope
		'' Clear the screen
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		SDL_RenderClear(renderer)
		'' Show the text already decoded
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)
		x = 0.0f
		y = 0.0f
		if lines then
			scope
				dim row_offset as long = ((row - rows) + 1)
				if (row_offset < 0) then
					scope
						row_offset = 0
					end scope
				end if
				scope
					i = 0
					do while (i < rows)
						scope
							dim line_ as Line_ ptr = lines[(((row_offset + i)) mod rows)]
							DisplayLine(x, y, line_)
							y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE
						end scope
						i += 1
					loop
				end scope
				'' Show the caption
				y = cast(single, ((((rows + 1)) * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)))
				if (progress = end_) then
					scope
						if (end_time = 0) then
							scope
								SDL_GetCurrentTime(@(end_time))
							end scope
						end if
						now = end_time
					end scope
				else
					scope
						SDL_GetCurrentTime(@(now))
					end scope
				end if
				elapsed = ((now - start_time))
				elapsed /= SDL_NS_PER_SECOND
				seconds = cast(long, ((elapsed mod 60)))
				elapsed /= 60
				minutes = cast(long, ((elapsed mod 60)))
				elapsed /= 60
				hours = cast(long, elapsed)
				SDL_asprintf(@(caption), strptr("Monkeys: %d - %dH:%dM:%dS"), cast(long, monkeys), cast(long, hours), cast(long, minutes), cast(long, seconds))
				if caption then
					scope
						SDL_RenderDebugText(renderer, x, y, caption)
						SDL_free(cptr(any ptr, caption))
					end scope
				end if
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE
				'' Show the characters currently typed
				DisplayLine(x, y, @(monkey_chars))
				y += SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE
			end scope
		end if
		'' Show the current progress
		SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE)
		rect.x = x
		rect.y = y
		rect.w = (((cast(single, ((progress - text))) / ((end_ - text)))) * ((cols * SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)))
		rect.h = cast(single, SDL_DEBUG_TEXT_FONT_CHARACTER_SIZE)
		SDL_RenderFillRect(renderer, @(rect))
		SDL_RenderPresent(renderer)
		return SDL_APP_CONTINUE
	end scope
end function

'' This function runs once at shutdown.
sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		'' SDL will clean up the window/renderer for us.
		FreeLines()
		SDL_free(cptr(any ptr, text))
	end scope
end sub

#include once "callback-main.bi"

'' end of infinite-monkeys.bas
