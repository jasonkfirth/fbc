'' Project: FreeBASIC SDL3 examples
'' File: woodeneye-008.bas
'' Purpose: Port upstream SDL3-3.4.18/examples/demo/02-woodeneye-008/woodeneye-008.c.
'' Responsibilities: Demonstrate the same SDL APIs and application lifecycle.
'' This file intentionally does NOT contain: compiler or library implementations.
''
'' Translated from the upstream C example; this is an altered source version.
'' This code is public domain. Feel free to use it for any purpose!

#include once "SDL3/SDL.bi"

#define MAP_BOX_SCALE 16
#define MAP_BOX_EDGES_LEN (12 + MAP_BOX_SCALE * 2)
#define MAX_PLAYER_COUNT 4
#define CIRCLE_DRAW_SIDES 32
#define CIRCLE_DRAW_SIDES_LEN (CIRCLE_DRAW_SIDES + 1)

type Player
	mouse as SDL_MouseID
	keyboard as SDL_KeyboardID
	pos_(0 to 2) as double
	vel(0 to 2) as double
	yaw as ulong
	pitch as long
	radius as single
	height as single
	color_(0 to 2) as ubyte
	wasd as ubyte
end type

type AppState
	window_ as SDL_Window ptr
	renderer as SDL_Renderer ptr
	player_count as long
	players(0 to 3) as Player
	edges(0 to 43, 0 to 5) as single
end type

type ExtendedMetadataRecord
	key as const zstring ptr
	value as const zstring ptr
end type

dim shared extended_metadata(0 to 3) as ExtendedMetadataRecord = {type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_URL_STRING), strptr("https://examples.libsdl.org/SDL3/demo/02-woodeneye-008/")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_CREATOR_STRING), strptr("SDL team")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_COPYRIGHT_STRING), strptr("Placed in the public domain")), type<ExtendedMetadataRecord>(strptr(SDL_PROP_APP_METADATA_TYPE_STRING), strptr("game"))}
dim shared debug_string(0 to 31) as byte

declare function whoseMouse cdecl(byval mouse as SDL_MouseID, byval players as const Player ptr, byval players_len as long) as long
declare function whoseKeyboard cdecl(byval keyboard as SDL_KeyboardID, byval players as const Player ptr, byval players_len as long) as long
declare sub shoot cdecl(byval shooter as long, byval players as Player ptr, byval players_len as long)
declare sub update cdecl(byval players as Player ptr, byval players_len as long, byval dt_ns as Uint64)
declare sub drawCircle cdecl(byval renderer as SDL_Renderer ptr, byval r as single, byval x as single, byval y as single)
declare sub drawClippedSegment cdecl(byval renderer as SDL_Renderer ptr, byval ax as single, byval ay as single, byval az as single, byval bx as single, byval by as single, byval bz as single, byval x as single, byval y as single, byval z as single, byval w as single)
declare sub draw_ cdecl(byval renderer as SDL_Renderer ptr, byval edges as single ptr, byval players as const Player ptr, byval players_len as long)
declare sub initPlayers cdecl(byval players as Player ptr, byval len_ as long)
declare sub initEdges cdecl(byval scale as long, byval edges as single ptr, byval edges_len as long)
declare function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
declare function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
declare function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
declare sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)

function whoseMouse cdecl(byval mouse as SDL_MouseID, byval players as const Player ptr, byval players_len as long) as long
	scope
		dim i as long
		scope
			i = 0
			do while (i < players_len)
				scope
					if (players[i].mouse = mouse) then
						return i
					end if
				end scope
				loop_continue_1:
				i += 1
			loop
		end scope
		return (-1)
	end scope
end function

function whoseKeyboard cdecl(byval keyboard as SDL_KeyboardID, byval players as const Player ptr, byval players_len as long) as long
	scope
		dim i as long
		scope
			i = 0
			do while (i < players_len)
				scope
					if (players[i].keyboard = keyboard) then
						return i
					end if
				end scope
				i += 1
			loop
		end scope
		return (-1)
	end scope
end function

sub shoot cdecl(byval shooter as long, byval players as Player ptr, byval players_len as long)
	scope
		dim i as long
		dim j as long
		dim x0 as double = players[shooter].pos_(0)
		dim y0 as double = players[shooter].pos_(1)
		dim z0 as double = players[shooter].pos_(2)
		dim bin_rad as double = (SDL_PI_D / 2147483648.0)
		dim yaw_rad as double = (bin_rad * players[shooter].yaw)
		dim pitch_rad as double = (bin_rad * players[shooter].pitch)
		dim cos_yaw as double = SDL_cos(yaw_rad)
		dim sin_yaw as double = SDL_sin(yaw_rad)
		dim cos_pitch as double = SDL_cos(pitch_rad)
		dim sin_pitch as double = SDL_sin(pitch_rad)
		dim vx as double = ((-sin_yaw) * cos_pitch)
		dim vy as double = sin_pitch
		dim vz as double = ((-cos_yaw) * cos_pitch)
		scope
			i = 0
			do while (i < players_len)
				scope
					if (i = shooter) then
						goto loop_continue_3
					end if
					dim target as Player ptr = @((players[i]))
					dim hit as long = 0
					scope
						j = 0
						do while (j < 2)
							scope
								dim r as double = cast(double, target->radius)
								dim h as double = cast(double, target->height)
								dim dx as double = (target->pos_(0) - x0)
								dim dy_ as double = ((target->pos_(1) - y0) + (iif((j = 0), 0, (r - h))))
								dim dz as double = (target->pos_(2) - z0)
								dim vd as double = (((vx * dx) + (vy * dy_)) + (vz * dz))
								dim dd as double = (((dx * dx) + (dy_ * dy_)) + (dz * dz))
								dim vv as double = (((vx * vx) + (vy * vy)) + (vz * vz))
								dim rr as double = (r * r)
								if (vd < 0) then
									goto loop_continue_4
								end if
								if ((vd * vd) >= (vv * ((dd - rr)))) then
									hit += 1
								end if
							end scope
							loop_continue_4:
							j += 1
						loop
					end scope
					if hit then
						scope
							target->pos_(0) = (cast(double, ((16 * ((SDL_rand(256) - 128))))) / 256)
							target->pos_(1) = (cast(double, ((16 * ((SDL_rand(256) - 128))))) / 256)
							target->pos_(2) = (cast(double, ((16 * ((SDL_rand(256) - 128))))) / 256)
						end scope
					end if
				end scope
				loop_continue_3:
				i += 1
			loop
		end scope
	end scope
end sub

sub update cdecl(byval players as Player ptr, byval players_len as long, byval dt_ns as Uint64)
	scope
		dim i as long
		scope
			i = 0
			do while (i < players_len)
				scope
					dim player as Player ptr = @(players[i])
					dim rate as double = 6.0
					dim time_ as double = (cast(double, dt_ns) * 1.0000000000000001E-9)
					dim drag as double = SDL_exp(((-time_) * rate))
					dim diff as double = (1.0 - drag)
					dim mult as double = 60.0
					dim grav as double = 25.0
					dim yaw as double = cast(double, player->yaw)
					dim rad as double = ((yaw * SDL_PI_D) / 2147483648.0)
					dim cos_ as double = SDL_cos(rad)
					dim sin_ as double = SDL_sin(rad)
					dim wasd as ubyte = player->wasd
					dim dirX as double = ((iif((wasd and 8), 1.0, 0.0)) - (iif((wasd and 2), 1.0, 0.0)))
					dim dirZ as double = ((iif((wasd and 4), 1.0, 0.0)) - (iif((wasd and 1), 1.0, 0.0)))
					dim norm as double = ((dirX * dirX) + (dirZ * dirZ))
					dim accX as double = (mult * (iif((norm = 0), 0, ((((cos_ * dirX) + (sin_ * dirZ))) / SDL_sqrt(norm)))))
					dim accZ as double = (mult * (iif((norm = 0), 0, (((((-sin_) * dirX) + (cos_ * dirZ))) / SDL_sqrt(norm)))))
					dim velX as double = player->vel(0)
					dim velY as double = player->vel(1)
					dim velZ as double = player->vel(2)
					player->vel(0) -= (velX * diff)
					player->vel(1) -= (grav * time_)
					player->vel(2) -= (velZ * diff)
					player->vel(0) += ((diff * accX) / rate)
					player->vel(2) += ((diff * accZ) / rate)
					player->pos_(0) += (((((time_ - (diff / rate))) * accX) / rate) + ((diff * velX) / rate))
					player->pos_(1) += (((((-0.5) * grav) * time_) * time_) + (velY * time_))
					player->pos_(2) += (((((time_ - (diff / rate))) * accZ) / rate) + ((diff * velZ) / rate))
					dim scale as double = cast(double, 16)
					dim bound as double = (scale - cast(double, player->radius))
					dim posX as double = (iif(((((iif((((bound) < (player->pos_(0)))), (bound), (player->pos_(0))))) > ((-bound)))), ((iif((((bound) < (player->pos_(0)))), (bound), (player->pos_(0))))), ((-bound))))
					dim posY as double = (iif(((((iif((((bound) < (player->pos_(1)))), (bound), (player->pos_(1))))) > ((cast(double, player->height) - scale)))), ((iif((((bound) < (player->pos_(1)))), (bound), (player->pos_(1))))), ((cast(double, player->height) - scale))))
					dim posZ as double = (iif(((((iif((((bound) < (player->pos_(2)))), (bound), (player->pos_(2))))) > ((-bound)))), ((iif((((bound) < (player->pos_(2)))), (bound), (player->pos_(2))))), ((-bound))))
					if (player->pos_(0) <> posX) then
						player->vel(0) = 0
					end if
					if (player->pos_(1) <> posY) then
						player->vel(1) = iif(((wasd and 16)), 8.4375, 0)
					end if
					if (player->pos_(2) <> posZ) then
						player->vel(2) = 0
					end if
					player->pos_(0) = posX
					player->pos_(1) = posY
					player->pos_(2) = posZ
				end scope
				i += 1
			loop
		end scope
	end scope
end sub

sub drawCircle cdecl(byval renderer as SDL_Renderer ptr, byval r as single, byval x as single, byval y as single)
	scope
		dim ang as single
		dim points(0 to 32) as SDL_FPoint
		dim i as long
		scope
			i = 0
			do while (i < ((32 + 1)))
				scope
					ang = (((2.0f * SDL_PI_F) * cast(single, i)) / cast(single, 32))
					points(i).x = (x + (r * SDL_cosf(ang)))
					points(i).y = (y + (r * SDL_sinf(ang)))
				end scope
				i += 1
			loop
		end scope
		SDL_RenderLines(renderer, cptr(const SDL_FPoint ptr, @points(0)), ((32 + 1)))
	end scope
end sub

sub drawClippedSegment cdecl(byval renderer as SDL_Renderer ptr, byval ax as single, byval ay as single, byval az as single, byval bx as single, byval by as single, byval bz as single, byval x as single, byval y as single, byval z as single, byval w as single)
	scope
		if ((az >= (-w)) andalso (bz >= (-w))) then
			exit sub
		end if
		dim dx as single = (ax - bx)
		dim dy_ as single = (ay - by)
		if (az > (-w)) then
			scope
				dim t as single = ((((-w) - bz)) / ((az - bz)))
				ax = (bx + (dx * t))
				ay = (by + (dy_ * t))
				az = (-w)
			end scope
		else
			if (bz > (-w)) then
				scope
					dim t as single = ((((-w) - az)) / ((bz - az)))
					bx = (ax - (dx * t))
					by = (ay - (dy_ * t))
					bz = (-w)
				end scope
			end if
		end if
		ax = (((-z) * ax) / az)
		ay = (((-z) * ay) / az)
		bx = (((-z) * bx) / bz)
		by = (((-z) * by) / bz)
		SDL_RenderLine(renderer, (x + ax), (y - ay), (x + bx), (y - by))
	end scope
end sub

sub draw_ cdecl(byval renderer as SDL_Renderer ptr, byval edges as single ptr, byval players as const Player ptr, byval players_len as long)
	scope
		dim w as long
		dim h as long
		dim i as long
		dim j as long
		dim k as long
		if (SDL_GetRenderOutputSize(renderer, @(w), @(h)) = 0) then
			scope
				exit sub
			end scope
		end if
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE)
		SDL_RenderClear(renderer)
		if (players_len > 0) then
			scope
				dim wf as single = cast(single, w)
				dim hf as single = cast(single, h)
				dim part_hor as long = iif((players_len > 2), 2, 1)
				dim part_ver as long = iif((players_len > 1), 2, 1)
				dim size_hor as single = (wf / (cast(single, part_hor)))
				dim size_ver as single = (hf / (cast(single, part_ver)))
				scope
					i = 0
					do while (i < players_len)
						scope
							dim player as const Player ptr = @(players[i])
							dim mod_x as single = cast(single, ((i mod part_hor)))
							dim mod_y as single = (cast(single, i) / part_hor)
							dim hor_origin as single = (((mod_x + 0.5f)) * size_hor)
							dim ver_origin as single = (((mod_y + 0.5f)) * size_ver)
							dim cam_origin as single = cast(single, ((0.5 * SDL_sqrt(cast(double, ((size_hor * size_hor) + (size_ver * size_ver)))))))
							dim hor_offset as single = (mod_x * size_hor)
							dim ver_offset as single = (mod_y * size_ver)
							dim rect as SDL_Rect
							rect.x = cast(long, hor_offset)
							rect.y = cast(long, ver_offset)
							rect.w = cast(long, size_hor)
							rect.h = cast(long, size_ver)
							SDL_SetRenderClipRect(renderer, @(rect))
							dim x0 as double = player->pos_(0)
							dim y0 as double = player->pos_(1)
							dim z0 as double = player->pos_(2)
							dim bin_rad as double = (SDL_PI_D / 2147483648.0)
							dim yaw_rad as double = (bin_rad * player->yaw)
							dim pitch_rad as double = (bin_rad * player->pitch)
							dim cos_yaw as double = SDL_cos(yaw_rad)
							dim sin_yaw as double = SDL_sin(yaw_rad)
							dim cos_pitch as double = SDL_cos(pitch_rad)
							dim sin_pitch as double = SDL_sin(pitch_rad)
							dim mat(0 to 8) as double = {cos_yaw, 0, (-sin_yaw), (sin_yaw * sin_pitch), cos_pitch, (cos_yaw * sin_pitch), (sin_yaw * cos_pitch), (-sin_pitch), (cos_yaw * cos_pitch)}
							SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255)
							scope
								k = 0
								do while (k < ((12 + (16 * 2))))
									scope
										dim line_ as const single ptr = (edges + (k) * 6)
										dim ax as single = cast(single, ((((mat(0) * ((cast(double, line_[0]) - x0))) + (mat(1) * ((cast(double, line_[1]) - y0)))) + (mat(2) * ((cast(double, line_[2]) - z0))))))
										dim ay as single = cast(single, ((((mat(3) * ((cast(double, line_[0]) - x0))) + (mat(4) * ((cast(double, line_[1]) - y0)))) + (mat(5) * ((cast(double, line_[2]) - z0))))))
										dim az as single = cast(single, ((((mat(6) * ((cast(double, line_[0]) - x0))) + (mat(7) * ((cast(double, line_[1]) - y0)))) + (mat(8) * ((cast(double, line_[2]) - z0))))))
										dim bx as single = cast(single, ((((mat(0) * ((cast(double, line_[3]) - x0))) + (mat(1) * ((cast(double, line_[4]) - y0)))) + (mat(2) * ((cast(double, line_[5]) - z0))))))
										dim by as single = cast(single, ((((mat(3) * ((cast(double, line_[3]) - x0))) + (mat(4) * ((cast(double, line_[4]) - y0)))) + (mat(5) * ((cast(double, line_[5]) - z0))))))
										dim bz as single = cast(single, ((((mat(6) * ((cast(double, line_[3]) - x0))) + (mat(7) * ((cast(double, line_[4]) - y0)))) + (mat(8) * ((cast(double, line_[5]) - z0))))))
										drawClippedSegment(renderer, ax, ay, az, bx, by, bz, hor_origin, ver_origin, cam_origin, 1)
									end scope
									k += 1
								loop
							end scope
							scope
								j = 0
								do while (j < players_len)
									scope
										if (i = j) then
											goto loop_continue_9
										end if
										dim target as const Player ptr = @(players[j])
										SDL_SetRenderDrawColor(renderer, target->color_(0), target->color_(1), target->color_(2), 255)
										scope
											k = 0
											do while (k < 2)
												scope
													dim rx as double = (target->pos_(0) - player->pos_(0))
													dim ry as double = ((target->pos_(1) - player->pos_(1)) + cast(double, (((target->radius - target->height)) * cast(single, k))))
													dim rz as double = (target->pos_(2) - player->pos_(2))
													dim dx as double = (((mat(0) * rx) + (mat(1) * ry)) + (mat(2) * rz))
													dim dy_ as double = (((mat(3) * rx) + (mat(4) * ry)) + (mat(5) * rz))
													dim dz as double = (((mat(6) * rx) + (mat(7) * ry)) + (mat(8) * rz))
													dim r_eff as double = (cast(double, (target->radius * cam_origin)) / dz)
													if (((dz < 0)) = 0) then
														goto loop_continue_10
													end if
													drawCircle(renderer, cast(single, (r_eff)), cast(single, ((cast(double, hor_origin) - ((cast(double, cam_origin) * dx) / dz)))), cast(single, ((cast(double, ver_origin) + ((cast(double, cam_origin) * dy_) / dz)))))
												end scope
												loop_continue_10:
												k += 1
											loop
										end scope
									end scope
									loop_continue_9:
									j += 1
								loop
							end scope
							SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
							SDL_RenderLine(renderer, hor_origin, (ver_origin - 10), hor_origin, (ver_origin + 10))
							SDL_RenderLine(renderer, (hor_origin - 10), ver_origin, (hor_origin + 10), ver_origin)
						end scope
						i += 1
					loop
				end scope
			end scope
		end if
		SDL_SetRenderClipRect(renderer, cptr(const SDL_Rect ptr, 0))
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderDebugText(renderer, 0, 0, @debug_string(0))
		SDL_RenderPresent(renderer)
	end scope
end sub

sub initPlayers cdecl(byval players as Player ptr, byval len_ as long)
	scope
		dim i as long
		scope
			i = 0
			do while (i < len_)
				scope
					players[i].pos_(0) = (8.0 * (iif((i and 1), (-1.0), 1.0)))
					players[i].pos_(1) = 0
					players[i].pos_(2) = ((8.0 * (iif((i and 1), (-1.0), 1.0))) * (iif((i and 2), (-1.0), 1.0)))
					players[i].vel(0) = 0
					players[i].vel(1) = 0
					players[i].vel(2) = 0
					players[i].yaw = ((536870912 + (iif((i and 1), 2147483648u, 0))) + (iif((i and 2), 1073741824, 0)))
					players[i].pitch = (-134217728)
					players[i].radius = 0.5f
					players[i].height = 1.5f
					players[i].wasd = 0
					players[i].mouse = 0
					players[i].keyboard = 0
					players[i].color_(0) = iif((((1 shl ((i \ 2)))) and 2), 0, 255)
					players[i].color_(1) = iif((((1 shl ((i \ 2)))) and 1), 0, 255)
					players[i].color_(2) = iif((((1 shl ((i \ 2)))) and 4), 0, 255)
					players[i].color_(0) = iif(((i and 1)), players[i].color_(0), (not players[i].color_(0)))
					players[i].color_(1) = iif(((i and 1)), players[i].color_(1), (not players[i].color_(1)))
					players[i].color_(2) = iif(((i and 1)), players[i].color_(2), (not players[i].color_(2)))
				end scope
				i += 1
			loop
		end scope
	end scope
end sub

sub initEdges cdecl(byval scale as long, byval edges as single ptr, byval edges_len as long)
	scope
		dim i as long
		dim j as long
		dim r as single = cast(single, scale)
		dim map(0 to 23) as long = {0, 1, 1, 3, 3, 2, 2, 0, 7, 6, 6, 4, 4, 5, 5, 7, 6, 2, 3, 7, 0, 4, 5, 1}
		scope
			i = 0
			do while (i < 12)
				scope
					scope
						j = 0
						do while (j < 3)
							scope
								(edges + (i) * 6)[(j + 0)] = (iif((map(((i * 2) + 0)) and ((1 shl j))), r, (-r)))
								(edges + (i) * 6)[(j + 3)] = (iif((map(((i * 2) + 1)) and ((1 shl j))), r, (-r)))
							end scope
							j += 1
						loop
					end scope
				end scope
				i += 1
			loop
		end scope
		scope
			i = 0
			do while (i < scale)
				scope
					dim d as single = cast(single, ((i * 2)))
					scope
						j = 0
						do while (j < 2)
							scope
								(edges + ((i + 12)) * 6)[((3 * j) + 0)] = iif(j, r, (-r))
								(edges + ((i + 12)) * 6)[((3 * j) + 1)] = (-r)
								(edges + ((i + 12)) * 6)[((3 * j) + 2)] = (d - r)
								(edges + (((i + 12) + scale)) * 6)[((3 * j) + 0)] = (d - r)
								(edges + (((i + 12) + scale)) * 6)[((3 * j) + 1)] = (-r)
								(edges + (((i + 12) + scale)) * 6)[((3 * j) + 2)] = iif(j, r, (-r))
							end scope
							j += 1
						loop
					end scope
				end scope
				i += 1
			loop
		end scope
	end scope
end sub

function SDL_AppInit cdecl(byval appstate as any ptr ptr, byval argc as long, byval argv as zstring ptr ptr) as SDL_AppResult
	scope
		if (SDL_SetAppMetadata(strptr("Example splitscreen shooter game"), strptr("1.0"), strptr("com.example.woodeneye-008")) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		dim i as long
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
		dim as_ as AppState ptr = cptr(AppState ptr, SDL_calloc(1, sizeof(*as_)))
		if (as_ = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		else
			scope
				(*appstate) = cptr(any ptr, as_)
			end scope
		end if
		if (SDL_Init(SDL_INIT_VIDEO) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		if (SDL_CreateWindowAndRenderer(strptr("examples/demo/woodeneye-008"), 640, 480, SDL_WINDOW_RESIZABLE, @(as_->window_), @(as_->renderer)) = 0) then
			scope
				return SDL_APP_FAILURE
			end scope
		end if
		as_->player_count = 1
		initPlayers(@as_->players(0), 4)
		initEdges(16, @as_->edges(0, 0), ((12 + (16 * 2))))
		cptr(byte ptr, @debug_string(0))[0] = 0
		SDL_SetRenderVSync(as_->renderer, 0)
		SDL_SetWindowRelativeMouseMode(as_->window_, true)
		SDL_SetHintWithPriority(strptr(SDL_HINT_WINDOWS_RAW_KEYBOARD), strptr("1"), SDL_HINT_OVERRIDE)
		return SDL_APP_CONTINUE
	end scope
end function

function SDL_AppEvent cdecl(byval appstate as any ptr, byval event as SDL_Event ptr) as SDL_AppResult
	scope
		dim as_ as AppState ptr = cptr(AppState ptr, appstate)
		dim players as Player ptr = @as_->players(0)
		dim player_count as long = as_->player_count
		dim i as long
		select case event->type
			case SDL_EVENT_QUIT
				goto switch_case_17
			case SDL_EVENT_MOUSE_REMOVED
				goto switch_case_18
			case SDL_EVENT_KEYBOARD_REMOVED
				goto switch_case_19
			case SDL_EVENT_MOUSE_MOTION
				goto switch_case_20
			case SDL_EVENT_MOUSE_BUTTON_DOWN
				goto switch_case_21
			case SDL_EVENT_KEY_DOWN
				goto switch_case_22
			case SDL_EVENT_KEY_UP
				goto switch_case_23
			case else
				goto switch_done_24
		end select
		switch_case_17:
		scope
			return SDL_APP_SUCCESS
			goto switch_done_24
		end scope
		switch_case_18:
		scope
			scope
				i = 0
				do while (i < player_count)
					scope
						if (players[i].mouse = event->mdevice.which) then
							scope
								players[i].mouse = 0
							end scope
						end if
					end scope
					i += 1
				loop
			end scope
			goto switch_done_24
		end scope
		switch_case_19:
		scope
			scope
				i = 0
				do while (i < player_count)
					scope
						if (players[i].keyboard = event->kdevice.which) then
							scope
								players[i].keyboard = 0
							end scope
						end if
					end scope
					i += 1
				loop
			end scope
			goto switch_done_24
		end scope
		switch_case_20:
		scope
			scope
				dim id as SDL_MouseID = event->motion.which
				dim index as long = whoseMouse(id, players, player_count)
				if (index >= 0) then
					scope
						players[index].yaw -= ((cast(long, event->motion.xrel)) * 524288)
						players[index].pitch = (iif(((((-1073741824)) > ((iif((((1073741824) < ((players[index].pitch - ((cast(long, event->motion.yrel)) * 524288))))), (1073741824), ((players[index].pitch - ((cast(long, event->motion.yrel)) * 524288)))))))), ((-1073741824)), ((iif((((1073741824) < ((players[index].pitch - ((cast(long, event->motion.yrel)) * 524288))))), (1073741824), ((players[index].pitch - ((cast(long, event->motion.yrel)) * 524288))))))))
					end scope
				else
					if id then
						scope
							scope
								i = 0
								do while (i < 4)
									scope
										if (players[i].mouse = 0) then
											scope
												players[i].mouse = id
												as_->player_count = (iif((((as_->player_count) > ((i + 1)))), (as_->player_count), ((i + 1))))
												exit do
											end scope
										end if
									end scope
									i += 1
								loop
							end scope
						end scope
					end if
				end if
				goto switch_done_24
			end scope
		end scope
		switch_case_21:
		scope
			scope
				dim id as SDL_MouseID = event->button.which
				dim index as long = whoseMouse(id, players, player_count)
				if (index >= 0) then
					scope
						shoot(index, players, player_count)
					end scope
				end if
				goto switch_done_24
			end scope
		end scope
		switch_case_22:
		scope
			scope
				dim sym as SDL_Keycode = event->key.key
				dim id as SDL_KeyboardID = event->key.which
				dim index as long = whoseKeyboard(id, players, player_count)
				if (index >= 0) then
					scope
						if (sym = 119u) then
							players[index].wasd or= 1
						end if
						if (sym = 97u) then
							players[index].wasd or= 2
						end if
						if (sym = 115u) then
							players[index].wasd or= 4
						end if
						if (sym = 100u) then
							players[index].wasd or= 8
						end if
						if (sym = 32u) then
							players[index].wasd or= 16
						end if
					end scope
				else
					if id then
						scope
							scope
								i = 0
								do while (i < 4)
									scope
										if (players[i].keyboard = 0) then
											scope
												players[i].keyboard = id
												as_->player_count = (iif((((as_->player_count) > ((i + 1)))), (as_->player_count), ((i + 1))))
												exit do
											end scope
										end if
									end scope
									i += 1
								loop
							end scope
						end scope
					end if
				end if
				goto switch_done_24
			end scope
		end scope
		switch_case_23:
		scope
			scope
				dim sym as SDL_Keycode = event->key.key
				dim id as SDL_KeyboardID = event->key.which
				if (sym = 27u) then
					return SDL_APP_SUCCESS
				end if
				dim index as long = whoseKeyboard(id, players, player_count)
				if (index >= 0) then
					scope
						if (sym = 119u) then
							players[index].wasd and= 30
						end if
						if (sym = 97u) then
							players[index].wasd and= 29
						end if
						if (sym = 115u) then
							players[index].wasd and= 27
						end if
						if (sym = 100u) then
							players[index].wasd and= 23
						end if
						if (sym = 32u) then
							players[index].wasd and= 15
						end if
					end scope
				end if
				goto switch_done_24
			end scope
		end scope
		switch_done_24:
		return SDL_APP_CONTINUE
	end scope
end function

function SDL_AppIterate cdecl(byval appstate as any ptr) as SDL_AppResult
	scope
		dim as_ as AppState ptr = cptr(AppState ptr, appstate)
		static accu as Uint64 = 0
		static last as Uint64 = 0
		static past as Uint64 = 0
		dim now as Uint64 = SDL_GetTicksNS()
		dim dt_ns as Uint64 = (now - past)
		update(@as_->players(0), as_->player_count, dt_ns)
		draw_(as_->renderer, cptr(single ptr, @as_->edges(0, 0)), @as_->players(0), as_->player_count)
		if ((now - last) > 999999999) then
			scope
				last = now
				SDL_snprintf(@debug_string(0), (sizeof(byte) * 32), strptr("%" SDL_PRIu64 " fps"), cast(Uint64, accu))
				accu = 0
			end scope
		end if
		past = now
		accu += 1
		dim elapsed as Uint64 = (SDL_GetTicksNS() - now)
		if (elapsed < 999999) then
			scope
				SDL_DelayNS((999999 - elapsed))
			end scope
		end if
		return SDL_APP_CONTINUE
	end scope
end function

sub SDL_AppQuit cdecl(byval appstate as any ptr, byval result as SDL_AppResult)
	scope
		SDL_free(appstate)
	end scope
end sub

#include once "callback-main.bi"

'' end of woodeneye-008.bas
