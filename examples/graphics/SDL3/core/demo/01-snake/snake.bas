'' Project: FreeBASIC SDL3 examples
'' File: snake.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/demo/01-snake/snake.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' Logic implementation of the Snake game. It is designed to efficiently
'' represent the state of the game in memory.
''
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define STEP_RATE_IN_MILLISECONDS 125
#define SNAKE_BLOCK_SIZE_IN_PIXELS 24
#define SDL_WINDOW_WIDTH (SNAKE_BLOCK_SIZE_IN_PIXELS * SNAKE_GAME_WIDTH)
#define SDL_WINDOW_HEIGHT (SNAKE_BLOCK_SIZE_IN_PIXELS * SNAKE_GAME_HEIGHT)
#define SNAKE_GAME_WIDTH 24U
#define SNAKE_GAME_HEIGHT 18U
#define SNAKE_MATRIX_SIZE (SNAKE_GAME_WIDTH * SNAKE_GAME_HEIGHT)
#define SNAKE_CELL_MAX_BITS 3U
#define SNAKE_CELL_SET_BITS (not (not 0u  shl  SNAKE_CELL_MAX_BITS))
#define SHIFT(x, y) (((x) + ((y) * SNAKE_GAME_WIDTH)) * SNAKE_CELL_MAX_BITS)

'' use the callbacks instead of main()
'' floor(log2(SNAKE_CELL_FOOD)) + 1
dim shared joystick as SDL_Joystick ptr = cptr(SDL_Joystick ptr, 0)
enum SnakeCell
	SNAKE_CELL_NOTHING = (0u)
	SNAKE_CELL_SRIGHT = (1u)
	SNAKE_CELL_SUP = (2u)
	SNAKE_CELL_SLEFT = (3u)
	SNAKE_CELL_SDOWN = (4u)
	SNAKE_CELL_FOOD = (5u)
end enum

enum SnakeDirection
	SNAKE_DIR_RIGHT
	SNAKE_DIR_UP
	SNAKE_DIR_LEFT
	SNAKE_DIR_DOWN
end enum

type SnakeContext
	cells(0 to 161) as ubyte
	head_xpos as byte
	head_ypos as byte
	tail_xpos as byte
	tail_ypos as byte
	next_dir as byte
	inhibit_tail_step as byte
	occupied_cells as ulong
end type

type AppState
	window_ as SDL_Window ptr
	renderer as SDL_Renderer ptr
	snake_ctx as SnakeContext
	last_step as Uint64
end type

type ExtendedMetadataRecord
	key as const zstring ptr
	value as const zstring ptr
end type

dim shared extended_metadata(0 to 3) as ExtendedMetadataRecord = {type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_URL_STRING), strptr("https://examples.libsdl.org/SDL3/demo/01-snake/")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_CREATOR_STRING), strptr("SDL team")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_COPYRIGHT_STRING), strptr("Placed in the public domain")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_TYPE_STRING), strptr("game"))}

declare function snake_cell_at cdecl(byval ctx as const SnakeContext ptr, byval x as byte, byval y as byte) as SnakeCell
declare sub set_rect_xy_ cdecl(byval r as SDL_FRect ptr, byval x as short, byval y as short)
declare sub put_cell_at_ cdecl(byval ctx as SnakeContext ptr, byval x as byte, byval y as byte, byval ct as SnakeCell)
declare function are_cells_full_ cdecl(byval ctx as SnakeContext ptr) as long
declare sub new_food_pos_ cdecl(byval ctx as SnakeContext ptr)
declare sub snake_initialize cdecl(byval ctx as SnakeContext ptr)
declare sub snake_redir cdecl(byval ctx as SnakeContext ptr, byval direction_index as SnakeDirection)
declare sub wrap_around_ cdecl(byval val_ as zstring ptr, byval max as byte)
declare sub snake_step cdecl(byval ctx as SnakeContext ptr)
declare function handle_key_event_ cdecl(byval ctx as SnakeContext ptr, byval key_code as SDL_Scancode) as SDL_AppResult
declare function handle_hat_event_ cdecl(byval ctx as SnakeContext ptr, byval hat as Uint8) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

function snake_cell_at cdecl(byval ctx as const SnakeContext ptr, byval x as byte, byval y as byte) as SnakeCell
	scope
		dim shift_bits as long = (((((x) + (((y) * 24u)))) * 3u))
		dim range as ushort
		SDL_memcpy(cptr(any ptr, @(range)), cptr(const any ptr, (@ctx->cells(0) + ((shift_bits \ 8)))), sizeof((range)))
		return cast(SnakeCell, ((((range shr ((shift_bits mod 8)))) and ((not (((not 0u) shl 3u)))))))
	end scope
end function

sub set_rect_xy_ cdecl(byval r as SDL_FRect ptr, byval x as short, byval y as short)
	scope
		r->x = cast(single, ((x * 24)))
		r->y = cast(single, ((y * 24)))
	end scope
end sub

sub put_cell_at_ cdecl(byval ctx as SnakeContext ptr, byval x as byte, byval y as byte, byval ct as SnakeCell)
	scope
		dim shift_bits as long = (((((x) + (((y) * 24u)))) * 3u))
		dim adjust as long = (shift_bits mod 8)
		dim pos_ as const ubyte ptr = (@ctx->cells(0) + ((shift_bits \ 8)))
		dim range as ushort
		SDL_memcpy(cptr(any ptr, @(range)), cptr(const any ptr, pos_), sizeof((range)))
		range and= (not ((((not (((not 0u) shl 3u)))) shl adjust)))
		'' clear bits
		range or= (((ct and ((not (((not 0u) shl 3u)))))) shl adjust)
		SDL_memcpy(cptr(any ptr, pos_), cptr(const any ptr, @(range)), sizeof((range)))
	end scope
end sub

function are_cells_full_ cdecl(byval ctx as SnakeContext ptr) as long
	scope
		return (ctx->occupied_cells = (24u * 18u))
	end scope
end function

sub new_food_pos_ cdecl(byval ctx as SnakeContext ptr)
	scope
		do
			if (1) = 0 then exit do
			scope
				dim x as byte = cast(byte, SDL_rand(24u))
				dim y as byte = cast(byte, SDL_rand(18u))
				if (snake_cell_at(ctx, x, y) = SNAKE_CELL_NOTHING) then
					scope
						put_cell_at_(ctx, x, y, SNAKE_CELL_FOOD)
						exit do
					end scope
				end if
			end scope
		loop
	end scope
end sub

sub snake_initialize cdecl(byval ctx as SnakeContext ptr)
	scope
		dim i as long
		SDL_memset(cptr(any ptr, @ctx->cells(0)), 0, (sizeof(ubyte) * 162))
		ctx->tail_xpos = (24u \ 2)
		ctx->head_xpos = ctx->tail_xpos
		ctx->tail_ypos = (18u \ 2)
		ctx->head_ypos = ctx->tail_ypos
		ctx->next_dir = SNAKE_DIR_RIGHT
		ctx->occupied_cells = 4
		ctx->inhibit_tail_step = ctx->occupied_cells
		ctx->occupied_cells -= 1
		put_cell_at_(ctx, ctx->tail_xpos, ctx->tail_ypos, SNAKE_CELL_SRIGHT)
		scope
			i = 0
			do while (i < 4)
				scope
					new_food_pos_(ctx)
					ctx->occupied_cells += 1
				end scope
				i += 1
			loop
		end scope
	end scope
end sub

sub snake_redir cdecl(byval ctx as SnakeContext ptr, byval direction_index as SnakeDirection)
	scope
		dim ct as SnakeCell = snake_cell_at(ctx, ctx->head_xpos, ctx->head_ypos)
		if ((((((direction_index = SNAKE_DIR_RIGHT) andalso (ct <> SNAKE_CELL_SLEFT))) orelse (((direction_index = SNAKE_DIR_UP) andalso (ct <> SNAKE_CELL_SDOWN)))) orelse (((direction_index = SNAKE_DIR_LEFT) andalso (ct <> SNAKE_CELL_SRIGHT)))) orelse (((direction_index = SNAKE_DIR_DOWN) andalso (ct <> SNAKE_CELL_SUP)))) then
			scope
				ctx->next_dir = direction_index
			end scope
		end if
	end scope
end sub

sub wrap_around_ cdecl(byval val_ as zstring ptr, byval max as byte)
	scope
		if ((*cptr(byte ptr, val_)) < 0) then
			scope
				(*cptr(byte ptr, val_)) = (max - 1)
			end scope
		else
			if ((*cptr(byte ptr, val_)) > (max - 1)) then
				scope
					(*cptr(byte ptr, val_)) = 0
				end scope
			end if
		end if
	end scope
end sub

sub snake_step cdecl(byval ctx as SnakeContext ptr)
	scope
		dim dir_as_cell as SnakeCell = cast(SnakeCell, ((ctx->next_dir + 1)))
		dim ct as SnakeCell
		dim prev_xpos as byte
		dim prev_ypos as byte
		'' Move tail forward
		ctx->inhibit_tail_step -= 1
		if (ctx->inhibit_tail_step = 0) then
			scope
				ctx->inhibit_tail_step += 1
				ct = snake_cell_at(ctx, ctx->tail_xpos, ctx->tail_ypos)
				put_cell_at_(ctx, ctx->tail_xpos, ctx->tail_ypos, SNAKE_CELL_NOTHING)
				select case ct
					case (SNAKE_CELL_SRIGHT)
						goto switch_case_3
					case (SNAKE_CELL_SUP)
						goto switch_case_4
					case (SNAKE_CELL_SLEFT)
						goto switch_case_5
					case (SNAKE_CELL_SDOWN)
						goto switch_case_6
					case else
						goto switch_case_7
				end select
				switch_case_3:
				scope
					ctx->tail_xpos += 1
					goto switch_done_8
				end scope
				switch_case_4:
				scope
					ctx->tail_ypos -= 1
					goto switch_done_8
				end scope
				switch_case_5:
				scope
					ctx->tail_xpos -= 1
					goto switch_done_8
				end scope
				switch_case_6:
				scope
					ctx->tail_ypos += 1
					goto switch_done_8
				end scope
				switch_case_7:
				scope
					goto switch_done_8
				end scope
				switch_done_8:
				wrap_around_(@(ctx->tail_xpos), 24u)
				wrap_around_(@(ctx->tail_ypos), 18u)
			end scope
		end if
		'' Move head forward
		prev_xpos = ctx->head_xpos
		prev_ypos = ctx->head_ypos
		select case ctx->next_dir
			case (SNAKE_DIR_RIGHT)
				goto switch_case_9
			case (SNAKE_DIR_UP)
				goto switch_case_10
			case (SNAKE_DIR_LEFT)
				goto switch_case_11
			case (SNAKE_DIR_DOWN)
				goto switch_case_12
			case else
				goto switch_case_13
		end select
		switch_case_9:
		scope
			ctx->head_xpos += 1
			goto switch_done_14
		end scope
		switch_case_10:
		scope
			ctx->head_ypos -= 1
			goto switch_done_14
		end scope
		switch_case_11:
		scope
			ctx->head_xpos -= 1
			goto switch_done_14
		end scope
		switch_case_12:
		scope
			ctx->head_ypos += 1
			goto switch_done_14
		end scope
		switch_case_13:
		scope
			goto switch_done_14
		end scope
		switch_done_14:
		wrap_around_(@(ctx->head_xpos), 24u)
		wrap_around_(@(ctx->head_ypos), 18u)
		'' Collisions
		ct = snake_cell_at(ctx, ctx->head_xpos, ctx->head_ypos)
		if ((ct <> SNAKE_CELL_NOTHING) andalso (ct <> SNAKE_CELL_FOOD)) then
			scope
				snake_initialize(ctx)
				exit sub
			end scope
		end if
		put_cell_at_(ctx, prev_xpos, prev_ypos, dir_as_cell)
		put_cell_at_(ctx, ctx->head_xpos, ctx->head_ypos, dir_as_cell)
		if (ct = SNAKE_CELL_FOOD) then
			scope
				if are_cells_full_(ctx) then
					scope
						snake_initialize(ctx)
						exit sub
					end scope
				end if
				new_food_pos_(ctx)
				ctx->inhibit_tail_step += 1
				ctx->occupied_cells += 1
			end scope
		end if
	end scope
end sub

function handle_key_event_ cdecl(byval ctx as SnakeContext ptr, byval key_code as SDL_Scancode) as SDL_AppResult
	scope
		select case key_code
			case SDL_SCANCODE_ESCAPE, SDL_SCANCODE_Q
				goto switch_case_15
			case SDL_SCANCODE_R
				goto switch_case_16
			case SDL_SCANCODE_RIGHT
				goto switch_case_17
			case SDL_SCANCODE_UP
				goto switch_case_18
			case SDL_SCANCODE_LEFT
				goto switch_case_19
			case SDL_SCANCODE_DOWN
				goto switch_case_20
			case else
				goto switch_case_21
		end select
		switch_case_15:
		scope
			'' Quit.
			return SDL_APP_SUCCESS
		end scope
		switch_case_16:
		scope
			'' Restart the game as if the program was launched.
			snake_initialize(ctx)
			goto switch_done_22
		end scope
		switch_case_17:
		scope
			'' Decide new direction of the snake.
			snake_redir(ctx, SNAKE_DIR_RIGHT)
			goto switch_done_22
		end scope
		switch_case_18:
		scope
			snake_redir(ctx, SNAKE_DIR_UP)
			goto switch_done_22
		end scope
		switch_case_19:
		scope
			snake_redir(ctx, SNAKE_DIR_LEFT)
			goto switch_done_22
		end scope
		switch_case_20:
		scope
			snake_redir(ctx, SNAKE_DIR_DOWN)
			goto switch_done_22
		end scope
		switch_case_21:
		scope
			goto switch_done_22
		end scope
		switch_done_22:
		return SDL_APP_CONTINUE
	end scope
end function

function handle_hat_event_ cdecl(byval ctx as SnakeContext ptr, byval hat as Uint8) as SDL_AppResult
	scope
		select case hat
			case SDL_HAT_RIGHT
				goto switch_case_23
			case SDL_HAT_UP
				goto switch_case_24
			case SDL_HAT_LEFT
				goto switch_case_25
			case SDL_HAT_DOWN
				goto switch_case_26
			case else
				goto switch_case_27
		end select
		switch_case_23:
		scope
			snake_redir(ctx, SNAKE_DIR_RIGHT)
			goto switch_done_28
		end scope
		switch_case_24:
		scope
			snake_redir(ctx, SNAKE_DIR_UP)
			goto switch_done_28
		end scope
		switch_case_25:
		scope
			snake_redir(ctx, SNAKE_DIR_LEFT)
			goto switch_done_28
		end scope
		switch_case_26:
		scope
			snake_redir(ctx, SNAKE_DIR_DOWN)
			goto switch_done_28
		end scope
		switch_case_27:
		scope
			goto switch_done_28
		end scope
		switch_done_28:
		return SDL_APP_CONTINUE
	end scope
end function

function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim as_ as AppState ptr = cptr(AppState ptr, appstate)
		dim ctx as SnakeContext ptr = @(as_->snake_ctx)
		dim now as Uint64 = SDL_GetTicks()
		dim r as SDL_FRect
		dim i as ulong
		dim j as ulong
		dim ct as long
		'' run game logic if we're at or past the time to run it.
		'' if we're _really_ behind the time to run it, run it
		'' several times.
		do
			if ((((now - as_->last_step)) >= 125)) = 0 then exit do
			scope
				snake_step(ctx)
				as_->last_step += 125
			end scope
		loop
		r.h = 24
		r.w = r.h
		SDL_SetRenderDrawColor(as_->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		SDL_RenderClear(as_->renderer)
		scope
			i = 0
			do while (i < 24u)
				scope
					scope
						j = 0
						do while (j < 18u)
							scope
								ct = snake_cell_at(ctx, i, j)
								if (ct = SNAKE_CELL_NOTHING) then
									goto loop_continue_31
								end if
								set_rect_xy_(@(r), i, j)
								if (ct = SNAKE_CELL_FOOD) then
									SDL_SetRenderDrawColor(as_->renderer, 80, 80, 255, SDL_ALPHA_OPAQUE)
								else
									'' body
									SDL_SetRenderDrawColor(as_->renderer, 0, 128, 0, SDL_ALPHA_OPAQUE)
								end if
								SDL_RenderFillRect(as_->renderer, @(r))
							end scope
							loop_continue_31:
							j += 1
						loop
					end scope
				end scope
				i += 1
			loop
		end scope
		SDL_SetRenderDrawColor(as_->renderer, 255, 255, 0, SDL_ALPHA_OPAQUE)
		'' head
		set_rect_xy_(@(r), ctx->head_xpos, ctx->head_ypos)
		SDL_RenderFillRect(as_->renderer, @(r))
		SDL_RenderPresent(as_->renderer)
		return SDL_APP_CONTINUE
	end scope
end function

function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		dim i as uinteger
		if (SDL_SetAppMetadata(strptr("Example Snake game"), strptr("1.0"), strptr("com.example.Snake")) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		scope
			i = 0
			do while (i < (((sizeof(ExtendedMetadataRecord) * 4) \ sizeof((extended_metadata(0))))))
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
		if (SDL_Init((SDL_INIT_VIDEO or SDL_INIT_JOYSTICK)) = 0) then
			scope
				SDL_Log_(strptr("Couldn't initialize SDL: %s"), SDL_GetError())
				return SDL_APP_FAILURE
			end scope
		end if
		dim as_ as AppState ptr = cptr(AppState ptr, SDL_calloc(1, sizeof(*as_)))
		if (as_ = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		(*appstate) = cptr(any ptr, as_)
		if (SDL_CreateWindowAndRenderer(strptr("examples/demo/snake"), (SDL_WINDOW_WIDTH), (SDL_WINDOW_HEIGHT), SDL_WINDOW_RESIZABLE, @(as_->window_), @(as_->renderer)) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		SDL_SetRenderLogicalPresentation(as_->renderer, (SDL_WINDOW_WIDTH), (SDL_WINDOW_HEIGHT), SDL_LOGICAL_PRESENTATION_LETTERBOX)
		snake_initialize(@(as_->snake_ctx))
		as_->last_step = SDL_GetTicks()
		return SDL_APP_CONTINUE
	end scope
end function

function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		dim ctx as SnakeContext ptr = @((cptr(AppState ptr, appstate))->snake_ctx)
		select case event->type
			case SDL_EVENT_QUIT
				goto switch_case_33
			case SDL_EVENT_JOYSTICK_ADDED
				goto switch_case_34
			case SDL_EVENT_JOYSTICK_REMOVED
				goto switch_case_35
			case SDL_EVENT_JOYSTICK_HAT_MOTION
				goto switch_case_36
			case SDL_EVENT_KEY_DOWN
				goto switch_case_37
			case else
				goto switch_case_38
		end select
		switch_case_33:
		scope
			return SDL_APP_SUCCESS
		end scope
		switch_case_34:
		scope
			if (joystick = cptr(SDL_Joystick ptr, (cptr(any ptr, 0)))) then
				scope
					joystick = SDL_OpenJoystick(event->jdevice.which)
					if (joystick = 0) then
						scope
							SDL_Log_(strptr("Failed to open joystick ID %u: %s"), cast(ulong, cast(ulong, event->jdevice.which)), SDL_GetError())
						end scope
					end if
				end scope
			end if
			goto switch_done_39
		end scope
		switch_case_35:
		scope
			if (joystick andalso ((SDL_GetJoystickID(joystick) = event->jdevice.which))) then
				scope
					SDL_CloseJoystick(joystick)
					joystick = cptr(SDL_Joystick ptr, 0)
				end scope
			end if
			goto switch_done_39
		end scope
		switch_case_36:
		scope
			return handle_hat_event_(ctx, event->jhat.value)
		end scope
		switch_case_37:
		scope
			return handle_key_event_(ctx, event->key.scancode)
		end scope
		switch_case_38:
		scope
			goto switch_done_39
		end scope
		switch_done_39:
		return SDL_APP_CONTINUE
	end scope
end function

sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		if joystick then
			scope
				SDL_CloseJoystick(joystick)
			end scope
		end if
		if (appstate <> (cptr(any ptr, 0))) then
			scope
				dim as_ as AppState ptr = cptr(AppState ptr, appstate)
				SDL_DestroyRenderer(as_->renderer)
				SDL_DestroyWindow(as_->window_)
				SDL_free(cptr(any ptr, as_))
			end scope
		end if
	end scope
end sub

#include once "callback-main.bi"

'' end of snake.bas
