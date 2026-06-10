''
'' FreeBASIC Sound Library (sfxlib)
'' --------------------------------
''
'' File: composer-grid.bas
''
'' Purpose:
''
''     Provide a mouse-driven music sketchpad for trying the generated
''     sound commands together with gfxlib drawing.
''
'' Responsibilities:
''
''     - draw a timeline grid and waveform palette with gfxlib primitives
''     - let the mouse place and erase generated sound events
''     - play the grid with sfxlib waveforms and pitched noise
''     - load, save, and export simple timeline files
''
'' This file intentionally does NOT contain:
''
''     - bitmap or media-file assets
''     - a full tracker or notation editor
''     - MIDI, sampled music, or external libraries
''

#include "fbgfx.bi"
#include once "sfxlib_raw.bi"

#if __FB_LANG__ = "fb"
Using FB
#endif

const SCREEN_W = 800
const SCREEN_H = 540

const GRID_X = 88
const GRID_Y = 68
const CELL_W = 18
const CELL_H = 24
const STEP_COUNT = 32
const PITCH_COUNT = 12
const INITIAL_SONG_STEPS = 64

const GRID_W = CELL_W * STEP_COUNT
const GRID_H = CELL_H * PITCH_COUNT

const SCROLL_X = GRID_X
const SCROLL_Y = GRID_Y + GRID_H + 14
const SCROLL_W = GRID_W
const SCROLL_H = 16

const PALETTE_X = 690
const PALETTE_Y = 68
const PALETTE_W = 92
const PALETTE_H = 48

const CONTROLS_Y = 390
const FILE_CONTROLS_Y = CONTROLS_Y + 36
const PREVIEW_X = 88
const PREVIEW_Y = 462
const PREVIEW_W = 444
const PREVIEW_H = 70
const SAMPLE_SFX_ID = 1

const KIND_EMPTY = 0
const KIND_SINE = 1
const KIND_SQUARE = 2
const KIND_TRIANGLE = 3
const KIND_SAW = 4
const KIND_NOISE = 5
const KIND_WAVEFILE = 6
const KIND_COUNT = 6

const ENTRY_NONE = 0
const ENTRY_WAVE = 1
const ENTRY_LOAD = 2
const ENTRY_SAVE = 3
const ENTRY_EXPORT_WAV = 4

const COLOR_BG = &h00f6f1df
const COLOR_PANEL = &h00fff7df
const COLOR_PANEL_DARK = &h00d4c59b
const COLOR_GRID = &h00ccf1e4
const COLOR_GRID_ALT = &h00b7e3da
const COLOR_GRID_LINE = &h0083aaa4
const COLOR_GRID_BAR = &h004a7770
const COLOR_TEXT = &h00242018
const COLOR_MUTED = &h00646a68
const COLOR_HEAD = &h00f7a21b
const COLOR_WHITE = &h00ffffff
const COLOR_BLACK = &h00000000

type UiMouse
	x as integer
	y as integer
	buttons as integer
	last_buttons as integer
end type

dim shared notes() as integer
dim shared pitch_name(0 to PITCH_COUNT - 1) as string
dim shared pitch_freq(0 to PITCH_COUNT - 1) as integer
dim shared kind_name(0 to KIND_COUNT) as string
dim shared kind_color(0 to KIND_COUNT) as uinteger
dim shared kind_instrument(0 to KIND_COUNT) as integer

dim shared selected_kind as integer = KIND_SINE
dim shared tempo_bpm as integer = 120
dim shared playing as integer = 0
dim shared quit_requested as integer = 0
dim shared wave_loaded as integer = 0
dim shared entry_mode as integer = ENTRY_NONE
dim shared song_steps as integer = INITIAL_SONG_STEPS
dim shared view_step as integer = 0
dim shared play_step as integer = 0
dim shared next_step_time as double = 0.0
dim shared last_paint_step as integer = -1
dim shared last_paint_pitch as integer = -1
dim shared scroll_dragging as integer = 0
dim shared scroll_drag_offset as integer = 0
dim shared wheel_z_set as integer = 0
dim shared wheel_w_set as integer = 0
dim shared last_wheel_z as integer = 0
dim shared last_wheel_w as integer = 0
dim shared wave_filename as string
dim shared arrangement_filename as string
dim shared export_filename as string
dim shared entry_text as string
dim shared status_text as string

declare function FileExists( byref filename as const string ) as integer
declare function DefaultWaveFilename() as string
declare function BaseName( byref filename as const string ) as string
declare function DisplayTail( byref text as const string, byval max_chars as integer ) as string
declare function NextToken( byref text as string ) as string
declare function ParseIntegerToken( byref text as string, byref value as integer ) as integer
declare function CellIndex( byval pitch as integer, byval column as integer ) as integer
declare function CellAt( byval pitch as integer, byval column as integer ) as integer
declare function LastUsedStep() as integer
declare function ArrangementLength() as integer
declare function ScrollLimit() as integer
declare function ScrollThumbWidth() as integer
declare function ScrollThumbX() as integer
declare sub InitTables()
declare sub InitSound()
declare sub SeedPattern()
declare function Inside( byval x as integer, byval y as integer, _
	byval rx as integer, byval ry as integer, _
	byval rw as integer, byval rh as integer ) as integer
declare function StepSeconds() as double
declare function NoteSeconds() as double
declare sub EnsureSongLength( byval min_steps as integer )
declare sub SetCell( byval pitch as integer, byval column as integer, byval kind as integer )
declare sub ScrollTo( byval column as integer )
declare sub ScrollBy( byval amount as integer )
declare sub ClearGrid()
declare sub TogglePlay()
declare sub StopPlayback()
declare sub TriggerCell( byval kind as integer, byval pitch as integer )
declare sub PlayColumn( byval column as integer )
declare sub UpdatePlayback()
declare sub HandleWheel( byref event_info as Event )
declare sub HandleMouse( byref mouse as UiMouse )
declare sub StartEntry( byval new_mode as integer )
declare function LoadWaveFilename( byref filename as const string, byval audition as integer ) as integer
declare sub LoadWaveFileEntry()
declare sub LoadArrangementEntry()
declare sub SaveArrangementEntry()
declare sub ExportSongWavEntry()
declare sub HandleEntryKey( byref event_info as Event )
declare sub DrawButton( byval x as integer, byval y as integer, _
	byval w as integer, byval h as integer, byval label as string, _
	byval active as integer )
declare sub DrawWaveIcon( byval kind as integer, byval x as integer, _
	byval y as integer, byval w as integer, byval h as integer, _
	byval draw_color as uinteger )
declare sub DrawPalette()
declare sub DrawPreview()
declare sub DrawGrid()
declare sub DrawScrollbar()
declare sub DrawControls()
declare sub DrawInterface()

function FileExists( byref filename as const string ) as integer
	dim as integer file_num

	if len(filename) = 0 then
		return 0
	end if

	file_num = freefile()

	if open( filename for binary access read as #file_num ) = 0 then
		close #file_num
		return -1
	end if

	return 0
end function

function DefaultWaveFilename() as string
	if FileExists( "media/buzzer.wav" ) then
		return "media/buzzer.wav"
	end if

	if FileExists( "examples/sfxlib/media/buzzer.wav" ) then
		return "examples/sfxlib/media/buzzer.wav"
	end if

	return "media/buzzer.wav"
end function

function BaseName( byref filename as const string ) as string
	dim as integer i
	dim as string ch

	for i = len(filename) to 1 step -1
		ch = mid(filename, i, 1)

		if ch = "/" or ch = "\" then
			return mid(filename, i + 1)
		end if
	next

	return filename
end function

function DisplayTail( byref text as const string, byval max_chars as integer ) as string
	if max_chars <= 3 then
		return left(text, max_chars)
	end if

	if len(text) <= max_chars then
		return text
	end if

	return "..." + right(text, max_chars - 3)
end function

function NextToken( byref text as string ) as string
	dim as integer space_pos
	dim as string token

	text = ltrim(text)

	if len(text) = 0 then
		return ""
	end if

	space_pos = instr(text, " ")

	if space_pos = 0 then
		token = text
		text = ""
	else
		token = left(text, space_pos - 1)
		text = ltrim(mid(text, space_pos + 1))
	end if

	return token
end function

function ParseIntegerToken( byref text as string, byref value as integer ) as integer
	dim as string token

	token = NextToken(text)

	if len(token) = 0 then
		return 0
	end if

	value = cint(val(token))
	return -1
end function

function CellIndex( byval pitch as integer, byval column as integer ) as integer
	return column * PITCH_COUNT + pitch
end function

function CellAt( byval pitch as integer, byval column as integer ) as integer
	if pitch < 0 or pitch >= PITCH_COUNT then
		return KIND_EMPTY
	end if

	if column < 0 or column >= song_steps then
		return KIND_EMPTY
	end if

	return notes(CellIndex(pitch, column))
end function

function LastUsedStep() as integer
	dim as integer column
	dim as integer row

	for column = song_steps - 1 to 0 step -1
		for row = 0 to PITCH_COUNT - 1
			if CellAt( row, column ) <> KIND_EMPTY then
				return column
			end if
		next
	next

	return 0
end function

function ArrangementLength() as integer
	dim as integer steps

	steps = LastUsedStep() + 1

	if steps < STEP_COUNT then
		steps = STEP_COUNT
	end if

	return steps
end function

function ScrollLimit() as integer
	if song_steps <= STEP_COUNT then
		return 0
	end if

	return song_steps - STEP_COUNT
end function

function ScrollThumbWidth() as integer
	dim as integer thumb_w

	if song_steps <= 0 then
		return SCROLL_W
	end if

	thumb_w = (SCROLL_W * STEP_COUNT) \ song_steps

	if thumb_w < 42 then
		thumb_w = 42
	end if

	if thumb_w > SCROLL_W then
		thumb_w = SCROLL_W
	end if

	return thumb_w
end function

function ScrollThumbX() as integer
	dim as integer limit
	dim as integer travel

	limit = ScrollLimit()

	if limit <= 0 then
		return SCROLL_X
	end if

	travel = SCROLL_W - ScrollThumbWidth()

	if travel <= 0 then
		return SCROLL_X
	end if

	return SCROLL_X + (view_step * travel) \ limit
end function

sub InitTables()
	redim notes(0 to song_steps * PITCH_COUNT - 1)

	pitch_name(0) = "C5" : pitch_freq(0) = 523
	pitch_name(1) = "B4" : pitch_freq(1) = 494
	pitch_name(2) = "A4" : pitch_freq(2) = 440
	pitch_name(3) = "G4" : pitch_freq(3) = 392
	pitch_name(4) = "F4" : pitch_freq(4) = 349
	pitch_name(5) = "E4" : pitch_freq(5) = 330
	pitch_name(6) = "D4" : pitch_freq(6) = 294
	pitch_name(7) = "C4" : pitch_freq(7) = 262
	pitch_name(8) = "B3" : pitch_freq(8) = 247
	pitch_name(9) = "A3" : pitch_freq(9) = 220
	pitch_name(10) = "G3" : pitch_freq(10) = 196
	pitch_name(11) = "F3" : pitch_freq(11) = 175

	kind_name(KIND_EMPTY) = "empty"
	kind_name(KIND_SINE) = "sine"
	kind_name(KIND_SQUARE) = "square"
	kind_name(KIND_TRIANGLE) = "triangle"
	kind_name(KIND_SAW) = "saw"
	kind_name(KIND_NOISE) = "noise"
	kind_name(KIND_WAVEFILE) = "wave file"

	kind_color(KIND_SINE) = &h001d8fd1
	kind_color(KIND_SQUARE) = &h00dc3d53
	kind_color(KIND_TRIANGLE) = &h003aa35c
	kind_color(KIND_SAW) = &h00e59f28
	kind_color(KIND_NOISE) = &h007651ba
	kind_color(KIND_WAVEFILE) = &h008c8c8c

	kind_instrument(KIND_SINE) = 1
	kind_instrument(KIND_SQUARE) = 2
	kind_instrument(KIND_TRIANGLE) = 3
	kind_instrument(KIND_SAW) = 4
	kind_instrument(KIND_NOISE) = 0
	kind_instrument(KIND_WAVEFILE) = 0

	wave_filename = DefaultWaveFilename()
	arrangement_filename = "composer-grid.txt"
	export_filename = "composer-grid.wav"
	entry_text = wave_filename
	status_text = "Click WAVE FILE to load a sample."
end sub

sub InitSound()
	dim as integer audio_channel

	volume 0.65

	wave 1, 0
	wave 2, 1
	wave 3, 2
	wave 4, 3

	envelope 1, 0.01, 0.03, 0.60, 0.09
	envelope 2, 0.00, 0.02, 0.55, 0.06
	envelope 3, 0.01, 0.05, 0.70, 0.10
	envelope 4, 0.00, 0.08, 0.40, 0.14

	instrument 1, 1, 1
	instrument 2, 2, 2
	instrument 3, 3, 3
	instrument 4, 4, 4

	for audio_channel = 0 to 15
		volume audio_channel, 0.90
		pan audio_channel, 0.00
	next
end sub

sub SeedPattern()
	ClearGrid()

	SetCell 7, 0, KIND_SINE
	SetCell 5, 2, KIND_SQUARE
	SetCell 4, 4, KIND_TRIANGLE
	SetCell 2, 6, KIND_SAW
	SetCell 7, 8, KIND_NOISE
	SetCell 5, 10, KIND_SINE
	SetCell 3, 12, KIND_SQUARE
	SetCell 2, 14, KIND_TRIANGLE

	SetCell 9, 0, KIND_SQUARE
	SetCell 9, 4, KIND_SQUARE
	SetCell 9, 8, KIND_SQUARE
	SetCell 9, 12, KIND_SQUARE

	SetCell 6, 16, KIND_SINE
	SetCell 4, 18, KIND_TRIANGLE
	SetCell 2, 20, KIND_SAW
	SetCell 0, 22, KIND_SINE
	SetCell 7, 24, KIND_NOISE
	SetCell 5, 26, KIND_SQUARE
	SetCell 4, 28, KIND_TRIANGLE
	SetCell 7, 30, KIND_SINE
end sub

function Inside( byval x as integer, byval y as integer, _
	byval rx as integer, byval ry as integer, _
	byval rw as integer, byval rh as integer ) as integer

	return (x >= rx) and (x < rx + rw) and (y >= ry) and (y < ry + rh)
end function

function StepSeconds() as double
	return 30.0 / cdbl( tempo_bpm )
end function

function NoteSeconds() as double
	return StepSeconds() * 0.82
end function

sub EnsureSongLength( byval min_steps as integer )
	dim as integer new_steps

	if min_steps <= song_steps then
		exit sub
	end if

	new_steps = song_steps

	if new_steps < 1 then
		new_steps = INITIAL_SONG_STEPS
	end if

	while new_steps < min_steps
		new_steps *= 2
	wend

	redim preserve notes(0 to new_steps * PITCH_COUNT - 1)
	song_steps = new_steps
end sub

sub SetCell( byval pitch as integer, byval column as integer, byval kind as integer )
	if pitch < 0 or pitch >= PITCH_COUNT then
		exit sub
	end if

	if column < 0 then
		exit sub
	end if

	if kind <> KIND_EMPTY then
		EnsureSongLength( column + 1 )
	elseif column >= song_steps then
		exit sub
	end if

	notes(CellIndex(pitch, column)) = kind
end sub

sub ScrollTo( byval column as integer )
	if column < 0 then
		column = 0
	end if

	if column > ScrollLimit() then
		column = ScrollLimit()
	end if

	view_step = column
end sub

sub ScrollBy( byval amount as integer )
	if amount > 0 and view_step + amount + STEP_COUNT >= song_steps - 4 then
		EnsureSongLength( view_step + amount + STEP_COUNT + 16 )
	end if

	ScrollTo( view_step + amount )
end sub

sub ClearGrid()
	dim as integer row, column

	for column = 0 to song_steps - 1
		for row = 0 to PITCH_COUNT - 1
			SetCell( row, column, KIND_EMPTY )
		next
	next

	play_step = 0
	view_step = 0
end sub

sub TogglePlay()
	playing = not playing

	if playing then
		next_step_time = timer
	else
		sfx stop
	end if
end sub

sub StopPlayback()
	playing = 0
	play_step = 0
	sfx stop
end sub

sub TriggerCell( byval kind as integer, byval pitch as integer )
	dim as integer audio_channel
	dim as integer frequency
	dim as double pan_value
	dim as single sample_pitch

	if kind <= KIND_EMPTY or kind > KIND_COUNT then
		exit sub
	end if

	if pitch < 0 or pitch >= PITCH_COUNT then
		exit sub
	end if

	audio_channel = pitch mod 12
	frequency = pitch_freq(pitch)
	pan_value = (cdbl((pitch mod 5) - 2) / 2.0) * 0.35

	pan audio_channel, pan_value

	if kind = KIND_WAVEFILE then
		if not wave_loaded then
			exit sub
		end if

		sample_pitch = csng(cdbl(frequency) / 262.0)

		if sample_pitch < 0.50 then
			sample_pitch = 0.50
		end if

		if sample_pitch > 2.00 then
			sample_pitch = 2.00
		end if

		sfx play audio_channel, SAMPLE_SFX_ID, sample_pitch
	elseif kind = KIND_NOISE then
		noise audio_channel, frequency * 8, NoteSeconds(), 0.35
	else
		instrument audio_channel, kind_instrument(kind)
		sound audio_channel, frequency, NoteSeconds(), 0.78
	end if
end sub

sub PlayColumn( byval column as integer )
	dim as integer row

	if column < 0 or column >= song_steps then
		exit sub
	end if

	for row = 0 to PITCH_COUNT - 1
		TriggerCell( CellAt( row, column ), row )
	next
end sub

sub UpdatePlayback()
	dim as double now_time
	dim as integer song_end

	if not playing then
		exit sub
	end if

	now_time = timer

	if now_time >= next_step_time then
		PlayColumn( play_step )
		play_step += 1

		song_end = LastUsedStep() + 1

		if song_end < STEP_COUNT then
			song_end = STEP_COUNT
		end if

		if play_step >= song_end then
			play_step = 0
		end if

		if play_step < view_step then
			ScrollTo( play_step )
		elseif play_step >= view_step + STEP_COUNT then
			ScrollTo( play_step - STEP_COUNT + 1 )
		end if

		next_step_time += StepSeconds()

		if now_time > next_step_time + StepSeconds() then
			next_step_time = now_time + StepSeconds()
		end if
	end if
end sub

sub HandleWheel( byref event_info as Event )
	dim as integer delta

	if entry_mode <> ENTRY_NONE then
		exit sub
	end if

	select case event_info.type
	case EVENT_MOUSE_WHEEL
		if not wheel_z_set then
			wheel_z_set = -1
		end if

		delta = event_info.z - last_wheel_z
		last_wheel_z = event_info.z

		if delta <> 0 then
			ScrollBy( -delta * 4 )
		end if

	case EVENT_MOUSE_HWHEEL
		if not wheel_w_set then
			wheel_w_set = -1
		end if

		delta = event_info.w - last_wheel_w
		last_wheel_w = event_info.w

		if delta <> 0 then
			ScrollBy( delta * 4 )
		end if
	end select
end sub

sub HandleMouse( byref mouse as UiMouse )
	dim as integer left_down
	dim as integer right_down
	dim as integer left_pressed
	dim as integer row
	dim as integer column
	dim as integer visible_column
	dim as integer slot
	dim as integer thumb_x
	dim as integer thumb_w
	dim as integer travel

	left_down = (mouse.buttons and 1) <> 0
	right_down = (mouse.buttons and 2) <> 0
	left_pressed = left_down and ((mouse.last_buttons and 1) = 0)

	if scroll_dragging then
		if left_down then
			thumb_w = ScrollThumbWidth()
			travel = SCROLL_W - thumb_w

			if travel > 0 then
				ScrollTo( ((mouse.x - SCROLL_X - scroll_drag_offset) * ScrollLimit()) \ travel )
			end if
		else
			scroll_dragging = 0
		end if

		exit sub
	end if

	if left_pressed then
		thumb_x = ScrollThumbX()
		thumb_w = ScrollThumbWidth()

		if Inside( mouse.x, mouse.y, SCROLL_X, SCROLL_Y, SCROLL_W, SCROLL_H ) then
			if Inside( mouse.x, mouse.y, thumb_x, SCROLL_Y, thumb_w, SCROLL_H ) then
				scroll_dragging = -1
				scroll_drag_offset = mouse.x - thumb_x
			else
				travel = SCROLL_W - thumb_w

				if travel > 0 then
					ScrollTo( ((mouse.x - SCROLL_X - thumb_w \ 2) * ScrollLimit()) \ travel )
				end if
			end if

			exit sub
		end if

		if Inside( mouse.x, mouse.y, 88, CONTROLS_Y, 66, 30 ) then
			TogglePlay()
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 164, CONTROLS_Y, 66, 30 ) then
			StopPlayback()
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 240, CONTROLS_Y, 66, 30 ) then
			ClearGrid()
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 88, FILE_CONTROLS_Y, 66, 30 ) then
			StartEntry( ENTRY_LOAD )
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 164, FILE_CONTROLS_Y, 66, 30 ) then
			StartEntry( ENTRY_SAVE )
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 240, FILE_CONTROLS_Y, 86, 30 ) then
			StartEntry( ENTRY_EXPORT_WAV )
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 560, CONTROLS_Y, 66, 30 ) then
			quit_requested = 1
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 406, CONTROLS_Y, 30, 30 ) then
			if tempo_bpm > 60 then
				tempo_bpm -= 10
			end if
			exit sub
		end if

		if Inside( mouse.x, mouse.y, 502, CONTROLS_Y, 30, 30 ) then
			if tempo_bpm < 220 then
				tempo_bpm += 10
			end if
			exit sub
		end if

		for slot = 0 to KIND_COUNT - 1
			if Inside( mouse.x, mouse.y, PALETTE_X, _
				PALETTE_Y + slot * (PALETTE_H + 8), PALETTE_W, PALETTE_H ) then

				if slot + 1 = KIND_WAVEFILE then
					if wave_loaded and selected_kind <> KIND_WAVEFILE then
						selected_kind = KIND_WAVEFILE
						TriggerCell( selected_kind, 7 )
					else
						StartEntry( ENTRY_WAVE )
					end if
				else
					if entry_mode <> ENTRY_NONE then
						entry_mode = ENTRY_NONE
					end if

					selected_kind = slot + 1
					TriggerCell( selected_kind, 7 )
				end if

				exit sub
			end if
		next
	end if

	if Inside( mouse.x, mouse.y, GRID_X, GRID_Y, GRID_W, GRID_H ) then
		if entry_mode <> ENTRY_NONE then
			exit sub
		end if

		visible_column = (mouse.x - GRID_X) \ CELL_W
		column = view_step + visible_column
		row = (mouse.y - GRID_Y) \ CELL_H

		if left_down then
			if row <> last_paint_pitch or column <> last_paint_step then
				if column >= song_steps - 4 then
					EnsureSongLength( column + 16 )
				end if

				SetCell row, column, selected_kind
				TriggerCell( selected_kind, row )
				last_paint_pitch = row
				last_paint_step = column
			end if
		elseif right_down then
			SetCell row, column, KIND_EMPTY
			last_paint_pitch = row
			last_paint_step = column
		end if
	end if

	if not left_down and not right_down then
		last_paint_pitch = -1
		last_paint_step = -1
	end if
end sub

sub StartEntry( byval new_mode as integer )
	StopPlayback()

	entry_mode = new_mode

	select case entry_mode
	case ENTRY_WAVE
		if len(wave_filename) > 0 then
			entry_text = wave_filename
		else
			entry_text = DefaultWaveFilename()
		end if

		status_text = "Type a WAV path and press Enter."

	case ENTRY_LOAD
		entry_text = arrangement_filename
		status_text = "Type a song file path and press Enter."

	case ENTRY_SAVE
		entry_text = arrangement_filename
		status_text = "Type a song file path and press Enter."

	case ENTRY_EXPORT_WAV
		entry_text = export_filename
		status_text = "Type a WAV export path and press Enter."

	case else
		entry_mode = ENTRY_NONE
	end select
end sub

function LoadWaveFilename( byref filename as const string, byval audition as integer ) as integer
	dim as string path

	path = trim(filename)

	if len(path) = 0 then
		wave_loaded = 0
		status_text = "No wave file selected."
		return 0
	end if

	if not FileExists( path ) then
		wave_loaded = 0
		status_text = "File not found: " + DisplayTail( path, 38 )
		return 0
	end if

	sfx stop
	sfx load SAMPLE_SFX_ID, path

	wave_filename = path
	wave_loaded = -1
	status_text = "Loaded " + DisplayTail( BaseName( wave_filename ), 38 )

	if audition then
		TriggerCell( KIND_WAVEFILE, 7 )
	end if

	return -1
end function

sub LoadWaveFileEntry()
	if LoadWaveFilename( entry_text, -1 ) then
		entry_mode = ENTRY_NONE
		selected_kind = KIND_WAVEFILE
	end if
end sub

sub LoadArrangementEntry()
	dim as integer file_num
	dim as string line_text
	dim as string parse_text
	dim as string tag
	dim as string loaded_wave
	dim as string load_status
	dim as integer column
	dim as integer row
	dim as integer kind
	dim as integer value

	entry_text = trim(entry_text)

	if len(entry_text) = 0 then
		status_text = "No song file selected."
		exit sub
	end if

	if not FileExists( entry_text ) then
		status_text = "File not found: " + DisplayTail( entry_text, 38 )
		exit sub
	end if

	file_num = freefile()

	if open( entry_text for input as #file_num ) <> 0 then
		status_text = "Could not open " + DisplayTail( entry_text, 38 )
		exit sub
	end if

	StopPlayback()

	song_steps = INITIAL_SONG_STEPS
	redim notes(0 to song_steps * PITCH_COUNT - 1)
	view_step = 0
	play_step = 0
	loaded_wave = wave_filename

	do while eof(file_num) = 0
		line input #file_num, line_text
		parse_text = trim(line_text)

		if len(parse_text) = 0 then
			continue do
		end if

		tag = lcase(NextToken(parse_text))

		select case tag
		case "sfx-composer-grid"
			'' Header line. The next token is the file-format version.

		case "tempo"
			if ParseIntegerToken( parse_text, value ) then
				if value < 60 then
					value = 60
				end if

				if value > 220 then
					value = 220
				end if

				tempo_bpm = value
			end if

		case "steps"
			if ParseIntegerToken( parse_text, value ) then
				if value < STEP_COUNT then
					value = STEP_COUNT
				end if

				EnsureSongLength( value )
			end if

		case "wave"
			loaded_wave = trim(parse_text)

		case "cell"
			if ParseIntegerToken( parse_text, column ) and _
				ParseIntegerToken( parse_text, row ) and _
				ParseIntegerToken( parse_text, kind ) then

				if column >= 0 and _
					row >= 0 and row < PITCH_COUNT and _
					kind >= KIND_EMPTY and kind <= KIND_COUNT then

					SetCell row, column, kind
				end if
			end if

		case "end"
			exit do
		end select
	loop

	close #file_num

	arrangement_filename = entry_text
	entry_mode = ENTRY_NONE
	load_status = "Loaded " + DisplayTail( BaseName( arrangement_filename ), 34 )

	if len(loaded_wave) > 0 then
		if LoadWaveFilename( loaded_wave, 0 ) then
			load_status += " with " + DisplayTail( BaseName( wave_filename ), 18 )
		else
			load_status += "; wave file missing"
		end if
	end if

	status_text = load_status
end sub

sub SaveArrangementEntry()
	dim as integer file_num
	dim as integer column
	dim as integer row
	dim as integer kind
	dim as integer saved_steps

	entry_text = trim(entry_text)

	if len(entry_text) = 0 then
		status_text = "No song file selected."
		exit sub
	end if

	file_num = freefile()

	if open( entry_text for output as #file_num ) <> 0 then
		status_text = "Could not save " + DisplayTail( entry_text, 38 )
		exit sub
	end if

	saved_steps = ArrangementLength()

	print #file_num, "sfx-composer-grid 1"
	print #file_num, "tempo "; tempo_bpm
	print #file_num, "steps "; saved_steps

	if len(wave_filename) > 0 then
		print #file_num, "wave "; wave_filename
	end if

	for column = 0 to saved_steps - 1
		for row = 0 to PITCH_COUNT - 1
			kind = CellAt( row, column )

			if kind <> KIND_EMPTY then
				print #file_num, "cell "; column; " "; row; " "; kind
			end if
		next
	next

	print #file_num, "end"
	close #file_num

	arrangement_filename = entry_text
	entry_mode = ENTRY_NONE
	status_text = "Saved " + DisplayTail( BaseName( arrangement_filename ), 38 )
end sub

sub ExportSongWavEntry()
	dim as string target
	dim as integer column
	dim as integer song_end
	dim as integer step_ms
	dim as integer tail_ms
	dim as long reserve_frames
	dim as double total_seconds

	target = trim(entry_text)

	if len(target) = 0 then
		status_text = "No WAV export file selected."
		exit sub
	end if

	StopPlayback()
	status_text = "Exporting WAV..."
	DrawInterface()

	sleep 80, 1

	if sfxlib.OutputCaptureStart() <> 0 then
		status_text = "Could not start output capture."
		exit sub
	end if

	song_end = ArrangementLength()
	total_seconds = cdbl(song_end) * StepSeconds() + NoteSeconds() + 0.35
	reserve_frames = clng(total_seconds * 48000.0) + 4096

	if reserve_frames > 0 then
		if sfxlib.OutputCaptureReserve( reserve_frames ) <> 0 then
			sfxlib.OutputCaptureStop()
			status_text = "Could not reserve WAV capture buffer."
			exit sub
		end if
	end if

	step_ms = cint(StepSeconds() * 1000.0)

	if step_ms < 1 then
		step_ms = 1
	end if

	for column = 0 to song_end - 1
		PlayColumn column
		sleep step_ms, 1
	next

	tail_ms = cint((NoteSeconds() + 0.20) * 1000.0)

	if tail_ms < 80 then
		tail_ms = 80
	end if

	sleep tail_ms, 1
	sfx stop
	sleep 40, 1

	sfxlib.OutputCaptureStop()

	if sfxlib.OutputCaptureSave( strptr( target ) ) <> 0 then
		status_text = "Could not save WAV: " + DisplayTail( target, 34 )
		exit sub
	end if

	export_filename = target
	entry_mode = ENTRY_NONE
	status_text = "Saved WAV " + DisplayTail( BaseName( export_filename ), 34 )
end sub

sub HandleEntryKey( byref event_info as Event )
	if event_info.scancode = SC_ESCAPE then
		entry_mode = ENTRY_NONE
		status_text = "File entry cancelled."
		exit sub
	end if

	if event_info.scancode = SC_ENTER then
		select case entry_mode
		case ENTRY_WAVE
			LoadWaveFileEntry()
		case ENTRY_LOAD
			LoadArrangementEntry()
		case ENTRY_SAVE
			SaveArrangementEntry()
		case ENTRY_EXPORT_WAV
			ExportSongWavEntry()
		end select

		exit sub
	end if

	if event_info.scancode = SC_BACKSPACE then
		if len(entry_text) > 0 then
			entry_text = left(entry_text, len(entry_text) - 1)
		end if

		exit sub
	end if

	if event_info.ascii >= 32 and event_info.ascii <= 126 then
		if len(entry_text) < 220 then
			entry_text += chr(event_info.ascii)
		end if
	end if
end sub

sub DrawButton( byval x as integer, byval y as integer, _
	byval w as integer, byval h as integer, byval label as string, _
	byval active as integer )

	dim as uinteger fill_color
	dim as uinteger border_color

	fill_color = iif( active, &h00ffd166, COLOR_PANEL )
	border_color = iif( active, COLOR_HEAD, COLOR_PANEL_DARK )

	line (x, y)-(x + w - 1, y + h - 1), fill_color, bf
	line (x, y)-(x + w - 1, y + h - 1), border_color, b
	draw string (x + (w - len(label) * 8) \ 2, y + 10), label, COLOR_TEXT
end sub

sub DrawWaveIcon( byval kind as integer, byval x as integer, _
	byval y as integer, byval w as integer, byval h as integer, _
	byval draw_color as uinteger )

	dim as integer cx, cy, amp
	dim as integer i, px, py, last_x, last_y
	dim as double angle

	cx = x + w \ 2
	cy = y + h \ 2
	amp = h \ 3

	select case kind
	case KIND_SINE
		for i = 0 to w - 1
			angle = cdbl(i) / cdbl(w - 1) * 6.283185307179586
			px = x + i
			py = cy + cint(sin(angle) * cdbl(amp))

			if i > 0 then
				line (last_x, last_y)-(px, py), draw_color
			end if

			last_x = px
			last_y = py
		next

	case KIND_SQUARE
		line (x, cy - amp)-(x + w \ 3, cy - amp), draw_color
		line (x + w \ 3, cy - amp)-(x + w \ 3, cy + amp), draw_color
		line (x + w \ 3, cy + amp)-(x + (w * 2) \ 3, cy + amp), draw_color
		line (x + (w * 2) \ 3, cy + amp)-(x + (w * 2) \ 3, cy - amp), draw_color
		line (x + (w * 2) \ 3, cy - amp)-(x + w, cy - amp), draw_color

	case KIND_TRIANGLE
		line (x, cy + amp)-(x + w \ 4, cy - amp), draw_color
		line -(x + (w * 3) \ 4, cy + amp), draw_color
		line -(x + w, cy - amp), draw_color

	case KIND_SAW
		line (x, cy + amp)-(x + w \ 3, cy - amp), draw_color
		line -(x + w \ 3, cy + amp), draw_color
		line -(x + (w * 2) \ 3, cy - amp), draw_color
		line -(x + (w * 2) \ 3, cy + amp), draw_color
		line -(x + w, cy - amp), draw_color

	case KIND_NOISE
		for i = 0 to 27
			px = x + ((i * 17 + 5) mod w)
			py = y + ((i * 11 + 3) mod h)
			pset (px, py), draw_color
		next
		line (x, cy)-(x + w, cy), draw_color, , &b1010101010101010

	case KIND_WAVEFILE
		line (x, y + h - 2)-(x + w, y + h - 2), draw_color
		line (x, y + h - 2)-(x + w \ 4, y + h \ 3), draw_color
		line -(x + w \ 2, y + h \ 2), draw_color
		line -(x + (w * 3) \ 4, y + h \ 4), draw_color
		line -(x + w, y + h - 2), draw_color
	end select
end sub

sub DrawPalette()
	dim as integer slot
	dim as integer kind
	dim as integer x
	dim as integer y

	draw string (PALETTE_X, 42), "WAVES", COLOR_TEXT

	for slot = 0 to KIND_COUNT - 1
		kind = slot + 1
		x = PALETTE_X
		y = PALETTE_Y + slot * (PALETTE_H + 8)

		if kind = KIND_WAVEFILE then
			line (x, y)-(x + PALETTE_W - 1, y + PALETTE_H - 1), COLOR_PANEL, bf
		else
			line (x, y)-(x + PALETTE_W - 1, y + PALETTE_H - 1), COLOR_PANEL, bf
		end if

		if kind = selected_kind then
			line (x, y)-(x + PALETTE_W - 1, y + PALETTE_H - 1), COLOR_HEAD, b
			line (x + 1, y + 1)-(x + PALETTE_W - 2, y + PALETTE_H - 2), COLOR_HEAD, b
		else
			line (x, y)-(x + PALETTE_W - 1, y + PALETTE_H - 1), COLOR_PANEL_DARK, b
		end if

		draw string (x + 9, y + 8), ucase(kind_name(kind)), COLOR_TEXT
		DrawWaveIcon kind, x + 10, y + 27, PALETTE_W - 20, 12, kind_color(kind)

		if kind = KIND_WAVEFILE then
			if wave_loaded then
				draw string (x + 31, y + 29), "ready", COLOR_MUTED
			else
				draw string (x + 35, y + 29), "load", COLOR_MUTED
			end if
		end if
	next
end sub

sub DrawPreview()
	dim as string prompt_label

	line (PREVIEW_X, PREVIEW_Y)-(PREVIEW_X + PREVIEW_W, PREVIEW_Y + PREVIEW_H), COLOR_PANEL, bf
	line (PREVIEW_X, PREVIEW_Y)-(PREVIEW_X + PREVIEW_W, PREVIEW_Y + PREVIEW_H), COLOR_PANEL_DARK, b

	if entry_mode <> ENTRY_NONE then
		select case entry_mode
		case ENTRY_WAVE
			prompt_label = "Wave file:"
		case ENTRY_LOAD
			prompt_label = "Load song:"
		case ENTRY_SAVE
			prompt_label = "Save song:"
		case ENTRY_EXPORT_WAV
			prompt_label = "Save WAV:"
		case else
			prompt_label = "File:"
		end select

		draw string (104, PREVIEW_Y + 14), prompt_label, COLOR_TEXT
		line (104, PREVIEW_Y + 32)-(516, PREVIEW_Y + 55), COLOR_WHITE, bf
		line (104, PREVIEW_Y + 32)-(516, PREVIEW_Y + 55), COLOR_PANEL_DARK, b
		draw string (112, PREVIEW_Y + 40), DisplayTail( entry_text, 48 ), COLOR_TEXT
		draw string (104, PREVIEW_Y + 60), "Enter accepts. Esc cancels.", COLOR_MUTED
	elseif selected_kind = KIND_WAVEFILE then
		draw string (104, PREVIEW_Y + 14), "Selected wave file:", COLOR_TEXT
		draw string (104, PREVIEW_Y + 30), DisplayTail( BaseName( wave_filename ), 46 ), COLOR_TEXT
		line (108, PREVIEW_Y + 55)-(512, PREVIEW_Y + 55), &h00ccbfa1
		DrawWaveIcon selected_kind, 116, PREVIEW_Y + 37, 388, 24, kind_color(selected_kind)
	else
		draw string (104, PREVIEW_Y + 14), "Selected waveform: " + kind_name(selected_kind), COLOR_TEXT
		line (108, PREVIEW_Y + 50)-(512, PREVIEW_Y + 50), &h00ccbfa1
		DrawWaveIcon selected_kind, 116, PREVIEW_Y + 29, 388, 36, kind_color(selected_kind)
	end if

	draw string (338, FILE_CONTROLS_Y + 10), DisplayTail( status_text, 52 ), COLOR_MUTED
end sub

sub DrawGrid()
	dim as integer row
	dim as integer column
	dim as integer actual_column
	dim as integer x
	dim as integer y
	dim as integer kind

	line (GRID_X - 1, GRID_Y - 1)-(GRID_X + GRID_W, GRID_Y + GRID_H), COLOR_BLACK, b

	for row = 0 to PITCH_COUNT - 1
		y = GRID_Y + row * CELL_H

		if (row and 1) = 0 then
			line (GRID_X, y)-(GRID_X + GRID_W - 1, y + CELL_H - 1), COLOR_GRID, bf
		else
			line (GRID_X, y)-(GRID_X + GRID_W - 1, y + CELL_H - 1), COLOR_GRID_ALT, bf
		end if

		draw string (GRID_X - 38, y + 8), pitch_name(row), COLOR_TEXT
	next

	for row = 0 to PITCH_COUNT
		y = GRID_Y + row * CELL_H
		line (GRID_X, y)-(GRID_X + GRID_W, y), COLOR_GRID_LINE
	next

	for column = 0 to STEP_COUNT
		x = GRID_X + column * CELL_W
		actual_column = view_step + column

		if (actual_column mod 4) = 0 then
			line (x, GRID_Y)-(x, GRID_Y + GRID_H), COLOR_GRID_BAR
		else
			line (x, GRID_Y)-(x, GRID_Y + GRID_H), COLOR_GRID_LINE, , &b1000100010001000
		end if

		if column < STEP_COUNT and (actual_column mod 8) = 0 then
			draw string (x + 2, GRID_Y - 18), str(actual_column + 1), COLOR_MUTED
		end if
	next

	for column = 0 to STEP_COUNT - 1
		actual_column = view_step + column

		for row = 0 to PITCH_COUNT - 1
			kind = CellAt( row, actual_column )

			if kind <> KIND_EMPTY then
				x = GRID_X + column * CELL_W
				y = GRID_Y + row * CELL_H
				circle (x + CELL_W \ 2, y + CELL_H \ 2), 8, COLOR_WHITE,,,, f
				circle (x + CELL_W \ 2, y + CELL_H \ 2), 8, kind_color(kind)
				DrawWaveIcon kind, x + 3, y + 7, CELL_W - 6, 10, kind_color(kind)
			end if
		next
	next

	if playing and play_step >= view_step and play_step < view_step + STEP_COUNT then
		x = GRID_X + (play_step - view_step) * CELL_W
		line (x, GRID_Y - 10)-(x, GRID_Y + GRID_H + 10), COLOR_HEAD
		line (x + 1, GRID_Y - 10)-(x + 1, GRID_Y + GRID_H + 10), COLOR_HEAD
	end if
end sub

sub DrawScrollbar()
	dim as integer thumb_x
	dim as integer thumb_w
	dim as string range_text

	thumb_x = ScrollThumbX()
	thumb_w = ScrollThumbWidth()

	line (SCROLL_X, SCROLL_Y)-(SCROLL_X + SCROLL_W - 1, SCROLL_Y + SCROLL_H - 1), COLOR_PANEL, bf
	line (SCROLL_X, SCROLL_Y)-(SCROLL_X + SCROLL_W - 1, SCROLL_Y + SCROLL_H - 1), COLOR_PANEL_DARK, b
	line (thumb_x, SCROLL_Y + 2)-(thumb_x + thumb_w - 1, SCROLL_Y + SCROLL_H - 3), COLOR_GRID_BAR, bf
	line (thumb_x, SCROLL_Y + 2)-(thumb_x + thumb_w - 1, SCROLL_Y + SCROLL_H - 3), COLOR_TEXT, b

	range_text = str(view_step + 1) + "-" + str(view_step + STEP_COUNT) + _
		" / " + str(LastUsedStep() + 1)

	draw string (SCROLL_X + SCROLL_W - len(range_text) * 8, SCROLL_Y + SCROLL_H + 5), _
		range_text, COLOR_MUTED
end sub

sub DrawControls()
	draw string (88, 42), "SFXLIB COMPOSER GRID", COLOR_TEXT
	draw string (88, 364), "Lay sounds on the timeline. Left paint, right erase.", COLOR_MUTED

	DrawButton 88, CONTROLS_Y, 66, 30, iif( playing, "PAUSE", "PLAY" ), playing
	DrawButton 164, CONTROLS_Y, 66, 30, "STOP", 0
	DrawButton 240, CONTROLS_Y, 66, 30, "CLEAR", 0
	DrawButton 560, CONTROLS_Y, 66, 30, "EXIT", 0

	DrawButton 88, FILE_CONTROLS_Y, 66, 30, "LOAD", entry_mode = ENTRY_LOAD
	DrawButton 164, FILE_CONTROLS_Y, 66, 30, "SAVE", entry_mode = ENTRY_SAVE
	DrawButton 240, FILE_CONTROLS_Y, 86, 30, "SAVE WAV", entry_mode = ENTRY_EXPORT_WAV

	draw string (338, CONTROLS_Y + 10), "TEMPO", COLOR_TEXT
	DrawButton 406, CONTROLS_Y, 30, 30, "-", 0
	line (442, CONTROLS_Y)-(496, CONTROLS_Y + 29), COLOR_PANEL, bf
	line (442, CONTROLS_Y)-(496, CONTROLS_Y + 29), COLOR_PANEL_DARK, b
	draw string (456, CONTROLS_Y + 10), str(tempo_bpm), COLOR_TEXT
	DrawButton 502, CONTROLS_Y, 30, 30, "+", 0
end sub

sub DrawInterface()
	screenlock
	cls
	line (0, 0)-(SCREEN_W - 1, SCREEN_H - 1), COLOR_BG, bf
	DrawGrid()
	DrawScrollbar()
	DrawPalette()
	DrawControls()
	DrawPreview()
	screenunlock
end sub

InitTables()
InitSound()
SeedPattern()

ScreenRes SCREEN_W, SCREEN_H, 32
WindowTitle "sfxlib composer grid"

dim as UiMouse mouse
dim as Event event_info
dim as integer mouse_available
dim as string key_text

do
	while ScreenEvent( @event_info )
		select case event_info.type
		case EVENT_WINDOW_CLOSE
			quit_requested = 1

		case EVENT_KEY_PRESS
			if entry_mode <> ENTRY_NONE then
				HandleEntryKey( event_info )
			else
				select case event_info.scancode
				case SC_ESCAPE
					quit_requested = 1
				case SC_SPACE
					TogglePlay()
				end select
			end if

		case EVENT_MOUSE_WHEEL, EVENT_MOUSE_HWHEEL
			HandleWheel( event_info )
		end select
	wend

	do
		key_text = inkey()

		if key_text = "" then
			exit do
		end if

		select case key_text
		case chr(255) + "k"
			quit_requested = 1
		case chr(27)
			if entry_mode = ENTRY_NONE then
				quit_requested = 1
			end if
		end select
	loop

	if quit_requested then
		exit do
	end if

	mouse.last_buttons = mouse.buttons
	mouse_available = GetMouse( mouse.x, mouse.y, , mouse.buttons )

	if mouse_available = 0 then
		HandleMouse mouse
	else
		mouse.buttons = 0
	end if

	UpdatePlayback()
	DrawInterface()

	sleep 12, 1
loop

sfx stop
screen 0, , , GFX_SCREEN_EXIT

'' end of composer-grid.bas
