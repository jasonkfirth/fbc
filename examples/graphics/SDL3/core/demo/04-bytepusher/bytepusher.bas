'' Project: FreeBASIC SDL3 examples
'' File: bytepusher.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/demo/04-bytepusher/bytepusher.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' An implementation of the BytePusher VM.
''
'' For example programs and more information about BytePusher, see
'' https://esolangs.org/wiki/BytePusher
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define SCREEN_W 256
#define SCREEN_H 256
#define RAM_SIZE &h1000000
#define FRAMES_PER_SECOND 60
#define SAMPLES_PER_FRAME 256
#define NS_PER_SECOND (Uint64)SDL_NS_PER_SECOND
#define MAX_AUDIO_LATENCY_FRAMES 5
#define IO_KEYBOARD 0
#define IO_PC 2
#define IO_SCREEN_PAGE 5
#define IO_AUDIO_BANK 6

type BytePusher
	ram(0 to 16777223) as Uint8
	last_tick as Uint64
	tick_acc as Uint64
	window_ as SDL_Window ptr
	renderer as SDL_Renderer ptr
	palette_ as SDL_Palette ptr
	texture as SDL_Texture ptr
	audiostream as SDL_AudioStream ptr
	status(0 to 31) as byte
	status_ticks as long
	keystate as Uint16
	display_help as boolean
	positional_input as boolean
end type

type ExtendedMetadataRecord
	key as const zstring ptr
	value as const zstring ptr
end type

dim shared extended_metadata(0 to 3) as ExtendedMetadataRecord = {type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_URL_STRING), strptr("https://examples.libsdl.org/SDL3/demo/04-bytepusher/")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_CREATOR_STRING), strptr("SDL team")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_COPYRIGHT_STRING), strptr("Placed in the public domain")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_TYPE_STRING), strptr("game"))}

declare function read_u16 cdecl(byval vm as const BytePusher ptr, byval addr as Uint32) as Uint16
declare function read_u24 cdecl(byval vm as const BytePusher ptr, byval addr as Uint32) as Uint32
declare sub set_status cdecl(byval vm as BytePusher ptr, byval fmt as const zstring ptr, ...)
declare function load cdecl(byval vm as BytePusher ptr, byval stream as SDL_IOStream ptr, byval closeio as boolean) as boolean
declare function filename cdecl(byval path as const zstring ptr) as const zstring ptr
declare function load_file cdecl(byval vm as BytePusher ptr, byval path as const zstring ptr) as boolean
declare sub print_ cdecl(byval vm as BytePusher ptr, byval x as long, byval y as long, byval str_ as const zstring ptr)
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare function keycode_mask cdecl(byval key as SDL_Keycode) as Uint16
declare function scancode_mask cdecl(byval scancode as SDL_Scancode) as Uint16
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

function read_u16 cdecl(byval vm as const BytePusher ptr, byval addr as Uint32) as Uint16
	scope
		dim ptr_ as const Uint8 ptr = @(vm->ram(addr))
		return (((cast(Uint16, ptr_[0]) shl 8)) or (cast(Uint16, ptr_[1])))
	end scope
end function

function read_u24 cdecl(byval vm as const BytePusher ptr, byval addr as Uint32) as Uint32
	scope
		dim ptr_ as const Uint8 ptr = @(vm->ram(addr))
		return ((((cast(Uint32, ptr_[0]) shl 16)) or ((cast(Uint32, ptr_[1]) shl 8))) or (cast(Uint32, ptr_[2])))
	end scope
end function

sub set_status cdecl(byval vm as BytePusher ptr, byval fmt as const zstring ptr, ...)
	scope
		dim args as va_list
		cva_start(args, fmt)
		SDL_vsnprintf(@vm->status(0), (sizeof(byte) * 32), fmt, args)
		cva_end(args)
		cptr(byte ptr, @vm->status(0))[((sizeof(byte) * 32) - 1)] = 0
		vm->status_ticks = (60 * 3)
	end scope
end sub

function load cdecl(byval vm as BytePusher ptr, byval stream as SDL_IOStream ptr, byval closeio as boolean) as boolean
	scope
		dim bytes_read as uinteger = 0
		dim ok as boolean = true
		vm->display_help = true
		'' will set to false if load succeeds.
		SDL_memset(cptr(any ptr, @vm->ram(0)), 0, 16777216)
		if (stream = 0) then
			scope
				return false
			end scope
		end if
		do
			if ((bytes_read < 16777216)) = 0 then exit do
			scope
				dim read_ as uinteger = SDL_ReadIO(stream, cptr(any ptr, @(vm->ram(bytes_read))), (16777216 - bytes_read))
				bytes_read += read_
				if (read_ = 0) then
					scope
						ok = cast(boolean, (SDL_GetIOStatus(stream) = SDL_IO_STATUS_EOF))
						exit do
					end scope
				end if
			end scope
		loop
		if closeio then
			scope
				SDL_CloseIO(stream)
			end scope
		end if
		SDL_ClearAudioStream(vm->audiostream)
		vm->display_help = cast(boolean, (ok = 0))
		return ok
	end scope
end function

function filename cdecl(byval path as const zstring ptr) as const zstring ptr
	scope
		dim i as uinteger = (SDL_strlen(path) + 1)
		do
			if ((i > 0)) = 0 then exit do
			scope
				i -= 1
				if ((cptr(byte ptr, path)[i] = 47) orelse (cptr(byte ptr, path)[i] = 92)) then
					scope
						return ((path + i) + 1)
					end scope
				end if
			end scope
		loop
		return path
	end scope
end function

function load_file cdecl(byval vm as BytePusher ptr, byval path as const zstring ptr) as boolean
	scope
		if load(vm, SDL_IOFromFile(path, strptr("rb")), true) then
			scope
				set_status(vm, strptr("loaded %s"), filename(path))
				return true
			end scope
		else
			scope
				set_status(vm, strptr("load failed: %s"), filename(path))
				return false
			end scope
		end if
	end scope
end function

sub print_ cdecl(byval vm as BytePusher ptr, byval x as long, byval y as long, byval str_ as const zstring ptr)
	scope
		SDL_SetRenderDrawColor(vm->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		SDL_RenderDebugText(vm->renderer, cast(single, ((x + 1))), cast(single, ((y + 1))), str_)
		SDL_SetRenderDrawColor(vm->renderer, 255, 255, 255, SDL_ALPHA_OPAQUE)
		SDL_RenderDebugText(vm->renderer, cast(single, x), cast(single, y), str_)
		SDL_SetRenderDrawColor(vm->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
	end scope
end sub

function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim vm as BytePusher ptr
		dim usable_bounds as SDL_Rect
		dim audiospec as SDL_AudioSpec = type<SDL_AudioSpec>(SDL_AUDIO_S8, 1, (256 * 60))
		dim primary_display as SDL_DisplayID
		dim zoom as long = 2
		dim i as long
		dim r as Uint8
		dim g as Uint8
		dim b as Uint8
		if (SDL_SetAppMetadata(strptr("SDL 3 BytePusher"), strptr("1.0"), strptr("com.example.SDL3BytePusher")) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		scope
			i = 0
			do while (i < cast(long, (((sizeof(ExtendedMetadataRecord) * 4) \ sizeof((extended_metadata(0)))))))
				scope
					if (SDL_SetAppMetadataProperty(extended_metadata(i).key, extended_metadata(i).value) = 0) then
						scope
							return SDL_APP_FAILURE
						end scope
					end if
				end scope
				i += 1
			loop
		end scope
		if (SDL_Init((SDL_INIT_AUDIO or SDL_INIT_VIDEO)) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		vm = cptr(BytePusher ptr, SDL_calloc(1, sizeof(((*vm)))))
		if ((vm) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		(*cptr(BytePusher ptr ptr, appstate)) = vm
		vm->display_help = true
		primary_display = SDL_GetPrimaryDisplay()
		if SDL_GetDisplayUsableBounds(primary_display, @(usable_bounds)) then
			scope
				dim zoom_w as long = (((((usable_bounds.w - usable_bounds.x)) * 2) \ 3) \ 256)
				dim zoom_h as long = (((((usable_bounds.h - usable_bounds.y)) * 2) \ 3) \ 256)
				zoom = iif((zoom_w < zoom_h), zoom_w, zoom_h)
				if (zoom < 1) then
					scope
						zoom = 1
					end scope
				end if
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("SDL 3 BytePusher"), (256 * zoom), (256 * zoom), SDL_WINDOW_RESIZABLE, @(vm->window_), @(vm->renderer)) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_SetRenderLogicalPresentation(vm->renderer, 256, 256, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		vm->palette_ = SDL_CreatePalette(256)
		if ((vm->palette_) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		i = 0
		scope
			r = 0
			do while (r < 6)
				scope
					scope
						g = 0
						do while (g < 6)
							scope
								scope
									b = 0
									do while (b < 6)
										scope
											dim color_ as SDL_Color = type<SDL_Color>(cast(Uint8, ((r * 51))), cast(Uint8, ((g * 51))), cast(Uint8, ((b * 51))), SDL_ALPHA_OPAQUE)
											vm->palette_->colors[i] = color_
										end scope
										b += 1
										i += 1
									loop
								end scope
							end scope
							g += 1
						loop
					end scope
				end scope
				r += 1
			loop
		end scope
		scope
			do while (i < 256)
				scope
					dim color_ as SDL_Color = type<SDL_Color>(0, 0, 0, SDL_ALPHA_OPAQUE)
					vm->palette_->colors[i] = color_
				end scope
				i += 1
			loop
		end scope
		vm->texture = SDL_CreateTexture(vm->renderer, SDL_PIXELFORMAT_INDEX8, SDL_TEXTUREACCESS_STREAMING, 256, 256)
		if (vm->texture = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetTexturePalette(vm->texture, vm->palette_)
		SDL_SetTextureScaleMode(vm->texture, SDL_SCALEMODE_NEAREST)
		vm->audiostream = SDL_OpenAudioDeviceStream((SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK), @(audiospec), cptr(SDL_AudioStreamCallback, 0), (cptr(any ptr, 0)))
		if ((vm->audiostream) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetAudioStreamGain(vm->audiostream, 0.100000001f)
		'' examples are loud!
		SDL_ResumeAudioStreamDevice(vm->audiostream)
		set_status(vm, strptr("renderer: %s"), SDL_GetRendererName(vm->renderer))
		vm->last_tick = SDL_GetTicksNS()
		vm->tick_acc = cast(Uint64, 1000000000ll)
		if (argc > 1) then
			scope
				load_file(vm, argv[1])
			end scope
		end if
		return SDL_APP_CONTINUE
	end scope
end function

function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim vm as BytePusher ptr = cptr(BytePusher ptr, appstate)
		dim tick as Uint64 = SDL_GetTicksNS()
		dim delta as Uint64 = (tick - vm->last_tick)
		dim updated as boolean
		dim skip_audio as boolean
		vm->last_tick = tick
		vm->tick_acc += (delta * 60)
		updated = cast(boolean, (vm->tick_acc >= cast(Uint64, 1000000000ll)))
		skip_audio = cast(boolean, (vm->tick_acc >= (5 * cast(Uint64, 1000000000ll))))
		if skip_audio then
			scope
				'' don't let audio fall too far behind
				SDL_ClearAudioStream(vm->audiostream)
			end scope
		end if
		do
			if ((vm->tick_acc >= cast(Uint64, 1000000000ll))) = 0 then exit do
			scope
				dim pc as Uint32
				dim i as long
				vm->tick_acc -= cast(Uint64, 1000000000ll)
				vm->ram(0) = cast(Uint8, ((vm->keystate shr 8)))
				vm->ram((0 + 1)) = cast(Uint8, (vm->keystate))
				pc = read_u24(vm, 2)
				scope
					i = 0
					do while (i < (256 * 256))
						scope
							dim src as Uint32 = read_u24(vm, pc)
							dim dst as Uint32 = read_u24(vm, (pc + 3))
							vm->ram(dst) = vm->ram(src)
							pc = read_u24(vm, (pc + 6))
						end scope
						i += 1
					loop
				end scope
				if ((skip_audio = 0) orelse (vm->tick_acc < cast(Uint64, 1000000000ll))) then
					scope
						SDL_PutAudioStreamData(vm->audiostream, cptr(const any ptr, @(vm->ram((cast(Uint32, read_u16(vm, 6)) shl 8)))), 256)
					end scope
				end if
			end scope
		loop
		if updated then
			scope
				dim pixels as const any ptr = cptr(const any ptr, @(vm->ram((cast(Uint32, vm->ram(5)) shl 16))))
				SDL_UpdateTexture(vm->texture, cptr(const SDL_Rect ptr, 0), pixels, 256)
			end scope
		end if
		SDL_RenderClear(vm->renderer)
		if vm->display_help then
			scope
				print_(vm, 4, 4, strptr("Drop a BytePusher file in this"))
				print_(vm, 8, 12, strptr("window to load and run it!"))
				print_(vm, 4, 28, strptr("Press ENTER to switch between"))
				print_(vm, 8, 36, strptr("positional and symbolic input."))
			end scope
		else
			scope
				SDL_RenderTexture(vm->renderer, vm->texture, cptr(const SDL_FRect ptr, 0), cptr(const SDL_FRect ptr, 0))
			end scope
		end if
		if (vm->status_ticks > 0) then
			scope
				if updated then
					scope
						vm->status_ticks -= 1
					end scope
				end if
				print_(vm, 4, (256 - 12), @vm->status(0))
			end scope
		end if
		SDL_RenderPresent(vm->renderer)
		return SDL_APP_CONTINUE
	end scope
end function

function keycode_mask cdecl(byval key as SDL_Keycode) as Uint16
	scope
		dim index as long
		if ((key >= 48u) andalso (key <= 57u)) then
			scope
				index = (key - 48u)
			end scope
		else
			if ((key >= 97u) andalso (key <= 102u)) then
				scope
					index = ((key - 97u) + 10)
				end scope
			else
				scope
					return 0
				end scope
			end if
		end if
		return (cast(Uint16, 1) shl index)
	end scope
end function

function scancode_mask cdecl(byval scancode as SDL_Scancode) as Uint16
	scope
		dim index as long
		select case scancode
			case SDL_SCANCODE_1
				goto switch_case_10
			case SDL_SCANCODE_2
				goto switch_case_11
			case SDL_SCANCODE_3
				goto switch_case_12
			case SDL_SCANCODE_4
				goto switch_case_13
			case SDL_SCANCODE_Q
				goto switch_case_14
			case SDL_SCANCODE_W
				goto switch_case_15
			case SDL_SCANCODE_E
				goto switch_case_16
			case SDL_SCANCODE_R
				goto switch_case_17
			case SDL_SCANCODE_A
				goto switch_case_18
			case SDL_SCANCODE_S
				goto switch_case_19
			case SDL_SCANCODE_D
				goto switch_case_20
			case SDL_SCANCODE_F
				goto switch_case_21
			case SDL_SCANCODE_Z
				goto switch_case_22
			case SDL_SCANCODE_X
				goto switch_case_23
			case SDL_SCANCODE_C
				goto switch_case_24
			case SDL_SCANCODE_V
				goto switch_case_25
			case else
				goto switch_case_26
		end select
		switch_case_10:
		scope
			index = 1
			goto switch_done_27
		end scope
		switch_case_11:
		scope
			index = 2
			goto switch_done_27
		end scope
		switch_case_12:
		scope
			index = 3
			goto switch_done_27
		end scope
		switch_case_13:
		scope
			index = 12
			goto switch_done_27
		end scope
		switch_case_14:
		scope
			index = 4
			goto switch_done_27
		end scope
		switch_case_15:
		scope
			index = 5
			goto switch_done_27
		end scope
		switch_case_16:
		scope
			index = 6
			goto switch_done_27
		end scope
		switch_case_17:
		scope
			index = 13
			goto switch_done_27
		end scope
		switch_case_18:
		scope
			index = 7
			goto switch_done_27
		end scope
		switch_case_19:
		scope
			index = 8
			goto switch_done_27
		end scope
		switch_case_20:
		scope
			index = 9
			goto switch_done_27
		end scope
		switch_case_21:
		scope
			index = 14
			goto switch_done_27
		end scope
		switch_case_22:
		scope
			index = 10
			goto switch_done_27
		end scope
		switch_case_23:
		scope
			index = 0
			goto switch_done_27
		end scope
		switch_case_24:
		scope
			index = 11
			goto switch_done_27
		end scope
		switch_case_25:
		scope
			index = 15
			goto switch_done_27
		end scope
		switch_case_26:
		scope
			return 0
		end scope
		switch_done_27:
		return (cast(Uint16, 1) shl index)
	end scope
end function

function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		dim vm as BytePusher ptr = cptr(BytePusher ptr, appstate)
		select case event->type
			case SDL_EVENT_QUIT
				goto switch_case_28
			case SDL_EVENT_DROP_FILE
				goto switch_case_29
			case SDL_EVENT_KEY_DOWN
				goto switch_case_30
			case SDL_EVENT_KEY_UP
				goto switch_case_31
			case else
				goto switch_done_32
		end select
		switch_case_28:
		scope
			return SDL_APP_SUCCESS
		end scope
		switch_case_29:
		scope
			load_file(vm, event->drop.data)
			goto switch_done_32
		end scope
		switch_case_30:
		scope
			if (event->key.key = 27u) then
				scope
					return SDL_APP_SUCCESS
				end scope
			end if
			if (event->key.key = 13u) then
				scope
					vm->positional_input = cast(boolean, (vm->positional_input = 0))
					vm->keystate = 0
					if vm->positional_input then
						scope
							set_status(vm, strptr("switched to positional input"))
						end scope
					else
						scope
							set_status(vm, strptr("switched to symbolic input"))
						end scope
					end if
				end scope
			end if
			if vm->positional_input then
				scope
					vm->keystate or= scancode_mask(event->key.scancode)
				end scope
			else
				scope
					vm->keystate or= keycode_mask(event->key.key)
				end scope
			end if
			goto switch_done_32
		end scope
		switch_case_31:
		scope
			if vm->positional_input then
				scope
					vm->keystate and= (not scancode_mask(event->key.scancode))
				end scope
			else
				scope
					vm->keystate and= (not keycode_mask(event->key.key))
				end scope
			end if
			goto switch_done_32
		end scope
		switch_done_32:
		return SDL_APP_CONTINUE
	end scope
end function

sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		if (result = SDL_APP_FAILURE) then
			scope
				SDL_Log_(strptr("Error: %s"), SDL_GetError())
			end scope
		end if
		if appstate then
			scope
				dim vm as BytePusher ptr = cptr(BytePusher ptr, appstate)
				SDL_DestroyAudioStream(vm->audiostream)
				SDL_DestroyTexture(vm->texture)
				SDL_DestroyPalette(vm->palette_)
				SDL_DestroyRenderer(vm->renderer)
				SDL_DestroyWindow(vm->window_)
				SDL_free(cptr(any ptr, vm))
			end scope
		end if
	end scope
end sub

#include once "callback-main.bi"

'' end of bytepusher.bas
