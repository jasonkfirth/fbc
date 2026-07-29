''
'' FreeBASIC Sound Library (sfxlib)
'' --------------------------------
''
'' File: midi-synth-player.bas
''
'' Purpose:
''
''     Play a Standard MIDI File through sfxlib's software oscillators rather
''     than forwarding MIDI messages to an operating-system synthesizer.
''
'' Responsibilities:
''
''     - parse format 0 and format 1 Standard MIDI Files
''     - merge tracks and convert tempo-map ticks to exact output frames
''     - pair note events while honoring sustain and common channel controls
''     - map General MIDI program families to sfxlib waves and envelopes
''     - synthesize channel 10 percussion with pitched noise
''     - report peak polyphony, dropped notes, clipping-friendly gain, and
''       output-driver underruns
''
'' This file intentionally does NOT contain:
''
''     - sample-based General MIDI instrument emulation
''     - an operating-system MIDI device dependency
''     - live MIDI input
''     - continuous retuning of notes already sounding during pitch bends
''

#include once "sfxlib_raw.bi"
#include once "sfxlib_effects.bi"

declare sub fb_sfxUpdate cdecl alias "fb_sfxUpdate" ( byval frames as long )
declare sub fb_sfxForegroundFeedBegin cdecl alias "fb_sfxForegroundFeedBegin" ( )
declare sub fb_sfxForegroundFeedEnd cdecl alias "fb_sfxForegroundFeedEnd" ( )
declare function fb_sfxVoiceActiveCount cdecl alias "fb_sfxVoiceActiveCount" ( ) as long

const MIDI_MAX_FILE_BYTES = 64 * 1024 * 1024
const MIDI_MAX_EVENTS = 1000000
const MIDI_MAX_CAPTURE_SECONDS = 120.0
const MIDI_DEFAULT_TEMPO_US = 500000
const MIDI_CHANNEL_COUNT = 16
const MIDI_KEY_COUNT = 128
const MIDI_DRUM_CHANNEL = 9

const MIDI_EVENT_NOTE_OFF = 1
const MIDI_EVENT_NOTE_ON = 2
const MIDI_EVENT_CONTROL = 3
const MIDI_EVENT_PROGRAM = 4
const MIDI_EVENT_PITCH = 5
const MIDI_EVENT_TEMPO = 6

type MidiEventRow
	tick as ulongint
	sequence as ulongint
	kind as ubyte
	channel as ubyte
	data1 as ubyte
	data2 as ubyte
	tempo_us as ulong
end type

type MidiNoteRow
	start_frame as longint
	end_frame as longint
	next_same_key as long
	channel as ubyte
	key_number as ubyte
	velocity as ubyte
	program_number as ubyte
	channel_volume as ubyte
	expression as ubyte
	pan_value as ubyte
	pitch_bend as ushort
	key_released as ubyte
end type

dim shared as ubyte midi_data()
dim shared as integer midi_data_size
dim shared as MidiEventRow midi_events()
dim shared as integer midi_event_count
dim shared as integer midi_event_capacity
dim shared as ulongint midi_event_sequence
dim shared as MidiNoteRow midi_notes()
dim shared as integer midi_note_count
dim shared as integer midi_note_capacity
dim shared as integer midi_active_head( 0 to MIDI_CHANNEL_COUNT - 1, _
	                                    0 to MIDI_KEY_COUNT - 1 )
dim shared as integer midi_program( 0 to MIDI_CHANNEL_COUNT - 1 )
dim shared as integer midi_channel_volume( 0 to MIDI_CHANNEL_COUNT - 1 )
dim shared as integer midi_expression( 0 to MIDI_CHANNEL_COUNT - 1 )
dim shared as integer midi_pan( 0 to MIDI_CHANNEL_COUNT - 1 )
dim shared as integer midi_pitch_bend( 0 to MIDI_CHANNEL_COUNT - 1 )
dim shared as integer midi_sustain( 0 to MIDI_CHANNEL_COUNT - 1 )
dim shared as string midi_error_text
dim shared as integer midi_unmatched_note_offs

'' -------------------------------------------------------------------------
'' Bounded binary readers
'' -------------------------------------------------------------------------

sub MidiSetError( byval text as string )
	if( midi_error_text = "" ) then
		midi_error_text = text
	end if
end sub

function MidiReadBe16( byval offset as integer ) as ushort
	return ( cushort( midi_data( offset ) ) shl 8 ) or _
	       cushort( midi_data( offset + 1 ) )
end function

function MidiReadBe32( byval offset as integer ) as ulong
	return ( culng( midi_data( offset ) ) shl 24 ) or _
	       ( culng( midi_data( offset + 1 ) ) shl 16 ) or _
	       ( culng( midi_data( offset + 2 ) ) shl 8 ) or _
	       culng( midi_data( offset + 3 ) )
end function

function MidiChunkMatches( byval offset as integer, byval tag_text as string ) as integer
	if( offset < 0 or offset + 4 > midi_data_size or len( tag_text ) <> 4 ) then
		return 0
	end if

	for index as integer = 0 to 3
		if( midi_data( offset + index ) <> asc( mid( tag_text, index + 1, 1 ) ) ) then
			return 0
		end if
	next

	return -1
end function

function MidiReadVariableLength _
	( _
		byref offset as integer, _
		byval limit as integer, _
		byref value as ulong _
	) as integer

	dim as ulongint result = 0

	for byte_index as integer = 0 to 3
		if( offset < 0 or offset >= limit ) then
			MidiSetError( "truncated variable-length MIDI value" )
			return -1
		end if

		dim as ubyte next_byte = midi_data( offset )
		offset += 1
		result = ( result shl 7 ) or culngint( next_byte and &h7f )

		if( ( next_byte and &h80 ) = 0 ) then
			value = culng( result )
			return 0
		end if
	next

	MidiSetError( "MIDI variable-length value exceeds four bytes" )
	return -1
end function

'' -------------------------------------------------------------------------
'' Event storage and track parsing
'' -------------------------------------------------------------------------

function MidiReserveEvents( byval required_count as integer ) as integer
	if( required_count < 0 or required_count > MIDI_MAX_EVENTS ) then
		MidiSetError( "MIDI event limit exceeded" )
		return -1
	end if

	if( required_count <= midi_event_capacity ) then
		return 0
	end if

	dim as integer new_capacity = midi_event_capacity
	if( new_capacity <= 0 ) then
		new_capacity = 1024
	end if

	while( new_capacity < required_count )
		if( new_capacity > MIDI_MAX_EVENTS \ 2 ) then
			new_capacity = MIDI_MAX_EVENTS
		else
			new_capacity *= 2
		end if
	wend

	redim preserve midi_events( 0 to new_capacity - 1 )
	midi_event_capacity = new_capacity
	return 0
end function

function MidiAppendEvent _
	( _
		byval tick as ulongint, _
		byval kind as integer, _
		byval channel_index as integer, _
		byval data1 as integer, _
		byval data2 as integer, _
		byval tempo_us as ulong = 0 _
	) as integer

	if( MidiReserveEvents( midi_event_count + 1 ) <> 0 ) then
		return -1
	end if

	with midi_events( midi_event_count )
		.tick = tick
		.sequence = midi_event_sequence
		.kind = cubyte( kind )
		.channel = cubyte( channel_index and &hf )
		.data1 = cubyte( data1 and &h7f )
		.data2 = cubyte( data2 and &h7f )
		.tempo_us = tempo_us
	end with

	midi_event_count += 1
	midi_event_sequence += 1
	return 0
end function

function MidiChannelMessageLength( byval status as integer ) as integer
	select case status and &hf0
	case &hc0, &hd0
		return 1
	case &h80, &h90, &ha0, &hb0, &he0
		return 2
	end select

	return 0
end function

function MidiParseTrack _
	( _
		byval track_offset as integer, _
		byval track_length as integer _
	) as integer

	dim as integer offset = track_offset
	dim as integer track_end = track_offset + track_length
	dim as integer running_status = 0
	dim as ulongint absolute_tick = 0

	while( offset < track_end )
		dim as ulong delta_tick
		if( MidiReadVariableLength( offset, track_end, delta_tick ) <> 0 ) then
			return -1
		end if
		absolute_tick += culngint( delta_tick )

		if( offset >= track_end ) then
			MidiSetError( "track ends before its MIDI event" )
			return -1
		end if

		dim as integer status = midi_data( offset )
		if( ( status and &h80 ) <> 0 ) then
			offset += 1
		else
			if( running_status = 0 ) then
				MidiSetError( "running status used before a channel status byte" )
				return -1
			end if
			status = running_status
		end if

		if( status = &hff ) then
			running_status = 0

			if( offset >= track_end ) then
				MidiSetError( "truncated MIDI meta event" )
				return -1
			end if

			dim as integer meta_type = midi_data( offset )
			offset += 1

			dim as ulong meta_length
			if( MidiReadVariableLength( offset, track_end, meta_length ) <> 0 ) then
				return -1
			end if

			if( culngint( meta_length ) > culngint( track_end - offset ) ) then
				MidiSetError( "MIDI meta event exceeds its track" )
				return -1
			end if

			if( meta_type = &h51 and meta_length = 3 ) then
				dim as ulong tempo_us = _
					( culng( midi_data( offset ) ) shl 16 ) or _
					( culng( midi_data( offset + 1 ) ) shl 8 ) or _
					culng( midi_data( offset + 2 ) )

				if( tempo_us = 0 ) then
					MidiSetError( "MIDI tempo event contains zero microseconds" )
					return -1
				end if

				if( MidiAppendEvent( absolute_tick, MIDI_EVENT_TEMPO, _
				                    0, 0, 0, tempo_us ) <> 0 ) then
					return -1
				end if
			end if

			offset += cint( meta_length )
			if( meta_type = &h2f ) then
				exit while
			end if
		elseif( status = &hf0 or status = &hf7 ) then
			running_status = 0

			dim as ulong sysex_length
			if( MidiReadVariableLength( offset, track_end, sysex_length ) <> 0 ) then
				return -1
			end if

			if( culngint( sysex_length ) > culngint( track_end - offset ) ) then
				MidiSetError( "MIDI system-exclusive event exceeds its track" )
				return -1
			end if

			offset += cint( sysex_length )
		else
			dim as integer message_length = MidiChannelMessageLength( status )
			if( message_length = 0 ) then
				MidiSetError( "unsupported MIDI status byte &h" + hex( status, 2 ) )
				return -1
			end if

			running_status = status
			if( offset + message_length > track_end ) then
				MidiSetError( "truncated MIDI channel message" )
				return -1
			end if

			dim as integer data1 = midi_data( offset )
			dim as integer data2 = 0
			offset += 1
			if( message_length = 2 ) then
				data2 = midi_data( offset )
				offset += 1
			end if

			if( data1 > 127 or data2 > 127 ) then
				MidiSetError( "channel message contains a status byte as data" )
				return -1
			end if

			dim as integer channel_index = status and &hf
			select case status and &hf0
			case &h80
				if( MidiAppendEvent( absolute_tick, MIDI_EVENT_NOTE_OFF, _
				                    channel_index, data1, data2 ) <> 0 ) then return -1
			case &h90
				if( data2 = 0 ) then
					if( MidiAppendEvent( absolute_tick, MIDI_EVENT_NOTE_OFF, _
					                    channel_index, data1, 0 ) <> 0 ) then return -1
				else
					if( MidiAppendEvent( absolute_tick, MIDI_EVENT_NOTE_ON, _
					                    channel_index, data1, data2 ) <> 0 ) then return -1
				end if
			case &hb0
				if( MidiAppendEvent( absolute_tick, MIDI_EVENT_CONTROL, _
				                    channel_index, data1, data2 ) <> 0 ) then return -1
			case &hc0
				if( MidiAppendEvent( absolute_tick, MIDI_EVENT_PROGRAM, _
				                    channel_index, data1, 0 ) <> 0 ) then return -1
			case &he0
				if( MidiAppendEvent( absolute_tick, MIDI_EVENT_PITCH, _
				                    channel_index, data1, data2 ) <> 0 ) then return -1
			end select
		end if
	wend

	return 0
end function

function MidiLoadFile _
	( _
		byval filename as string, _
		byref format_number as integer, _
		byref track_count as integer, _
		byref division as integer _
	) as integer

	midi_error_text = ""
	midi_event_count = 0
	midi_event_capacity = 0
	midi_event_sequence = 0
	erase midi_events

	dim as integer file_number = freefile()
	if( open( filename for binary access read as #file_number ) <> 0 ) then
		MidiSetError( "could not open MIDI file: " + filename )
		return -1
	end if

	dim as longint file_size = lof( file_number )
	if( file_size < 14 or file_size > MIDI_MAX_FILE_BYTES ) then
		close #file_number
		MidiSetError( "MIDI file size is outside the supported range" )
		return -1
	end if

	midi_data_size = cint( file_size )
	redim midi_data( 0 to midi_data_size - 1 )
	get #file_number, 1, midi_data()
	close #file_number

	if( MidiChunkMatches( 0, "MThd" ) = 0 ) then
		MidiSetError( "file does not begin with an MThd chunk" )
		return -1
	end if

	dim as ulong header_length = MidiReadBe32( 4 )
	if( header_length < 6 or culngint( header_length ) > _
	    culngint( midi_data_size - 8 ) ) then
		MidiSetError( "invalid MIDI header length" )
		return -1
	end if

	format_number = MidiReadBe16( 8 )
	track_count = MidiReadBe16( 10 )
	division = MidiReadBe16( 12 )

	if( format_number < 0 or format_number > 1 ) then
		MidiSetError( "only simultaneous format 0 and format 1 MIDI files are supported" )
		return -1
	end if

	if( track_count <= 0 or track_count > 256 ) then
		MidiSetError( "MIDI track count is outside the supported range" )
		return -1
	end if

	if( ( division and &h8000 ) <> 0 or division = 0 ) then
		MidiSetError( "SMPTE-time MIDI division is not supported" )
		return -1
	end if

	dim as integer offset = 8 + cint( header_length )
	for track_index as integer = 0 to track_count - 1
		if( offset < 0 or offset + 8 > midi_data_size ) then
			MidiSetError( "MIDI file ends before all declared tracks" )
			return -1
		end if

		if( MidiChunkMatches( offset, "MTrk" ) = 0 ) then
			MidiSetError( "declared MIDI track does not begin with MTrk" )
			return -1
		end if

		dim as ulong track_length = MidiReadBe32( offset + 4 )
		offset += 8

		if( culngint( track_length ) > culngint( midi_data_size - offset ) ) then
			MidiSetError( "MIDI track length exceeds the file" )
			return -1
		end if

		if( MidiParseTrack( offset, cint( track_length ) ) <> 0 ) then
			return -1
		end if

		offset += cint( track_length )
	next

	return 0
end function

'' -------------------------------------------------------------------------
'' Stable event ordering
'' -------------------------------------------------------------------------

function MidiCompareEvents _
	( _
		byref left_row as MidiEventRow, _
		byref right_row as MidiEventRow _
	) as integer

	if( left_row.tick < right_row.tick ) then return -1
	if( left_row.tick > right_row.tick ) then return 1
	if( left_row.sequence < right_row.sequence ) then return -1
	if( left_row.sequence > right_row.sequence ) then return 1
	return 0
end function

sub MidiSortEvents( byval low_index as integer, byval high_index as integer )
	dim as integer left_index = low_index
	dim as integer right_index = high_index
	dim as MidiEventRow pivot = midi_events( low_index + ( high_index - low_index ) \ 2 )

	do
		while( left_index <= high_index andalso _
		       MidiCompareEvents( midi_events( left_index ), pivot ) < 0 )
			left_index += 1
		wend

		while( right_index >= low_index andalso _
		       MidiCompareEvents( midi_events( right_index ), pivot ) > 0 )
			right_index -= 1
		wend

		if( left_index <= right_index ) then
			swap midi_events( left_index ), midi_events( right_index )
			left_index += 1
			right_index -= 1
		end if
	loop while( left_index <= right_index )

	if( low_index < right_index ) then MidiSortEvents( low_index, right_index )
	if( left_index < high_index ) then MidiSortEvents( left_index, high_index )
end sub

'' -------------------------------------------------------------------------
'' Note pairing and controller state
'' -------------------------------------------------------------------------

function MidiReserveNotes( byval required_count as integer ) as integer
	if( required_count < 0 or required_count > MIDI_MAX_EVENTS ) then
		MidiSetError( "MIDI note limit exceeded" )
		return -1
	end if

	if( required_count <= midi_note_capacity ) then return 0

	dim as integer new_capacity = midi_note_capacity
	if( new_capacity <= 0 ) then new_capacity = 512

	while( new_capacity < required_count )
		if( new_capacity > MIDI_MAX_EVENTS \ 2 ) then
			new_capacity = MIDI_MAX_EVENTS
		else
			new_capacity *= 2
		end if
	wend

	redim preserve midi_notes( 0 to new_capacity - 1 )
	midi_note_capacity = new_capacity
	return 0
end function

function MidiStartNote _
	( _
		byval channel_index as integer, _
		byval key_number as integer, _
		byval velocity as integer, _
		byval start_frame as longint _
	) as integer

	if( MidiReserveNotes( midi_note_count + 1 ) <> 0 ) then return -1

	with midi_notes( midi_note_count )
		.start_frame = start_frame
		.end_frame = -1
		.next_same_key = midi_active_head( channel_index, key_number )
		.channel = cubyte( channel_index )
		.key_number = cubyte( key_number )
		.velocity = cubyte( velocity )
		.program_number = cubyte( midi_program( channel_index ) )
		.channel_volume = cubyte( midi_channel_volume( channel_index ) )
		.expression = cubyte( midi_expression( channel_index ) )
		.pan_value = cubyte( midi_pan( channel_index ) )
		.pitch_bend = cushort( midi_pitch_bend( channel_index ) )
		.key_released = 0
	end with

	midi_active_head( channel_index, key_number ) = midi_note_count
	midi_note_count += 1
	return 0
end function

sub MidiReleaseOneNote _
	( _
		byval channel_index as integer, _
		byval key_number as integer, _
		byval end_frame as longint _
	)

	dim as integer previous_index = -1
	dim as integer note_index = midi_active_head( channel_index, key_number )

	while( note_index >= 0 )
		if( midi_notes( note_index ).key_released = 0 ) then exit while
		previous_index = note_index
		note_index = midi_notes( note_index ).next_same_key
	wend

	if( note_index < 0 ) then
		midi_unmatched_note_offs += 1
		exit sub
	end if

	if( midi_sustain( channel_index ) <> 0 ) then
		midi_notes( note_index ).key_released = 1
		exit sub
	end if

	midi_notes( note_index ).end_frame = end_frame
	if( midi_notes( note_index ).end_frame <= midi_notes( note_index ).start_frame ) then
		midi_notes( note_index ).end_frame = midi_notes( note_index ).start_frame + 1
	end if

	if( previous_index < 0 ) then
		midi_active_head( channel_index, key_number ) = _
			midi_notes( note_index ).next_same_key
	else
		midi_notes( previous_index ).next_same_key = _
			midi_notes( note_index ).next_same_key
	end if
end sub

sub MidiReleaseSustained( byval channel_index as integer, byval end_frame as longint )
	for key_number as integer = 0 to MIDI_KEY_COUNT - 1
		dim as integer previous_index = -1
		dim as integer note_index = midi_active_head( channel_index, key_number )

		while( note_index >= 0 )
			dim as integer next_index = midi_notes( note_index ).next_same_key

			if( midi_notes( note_index ).key_released <> 0 ) then
				midi_notes( note_index ).end_frame = end_frame
				if( midi_notes( note_index ).end_frame <= _
				    midi_notes( note_index ).start_frame ) then
					midi_notes( note_index ).end_frame = _
						midi_notes( note_index ).start_frame + 1
				end if

				if( previous_index < 0 ) then
					midi_active_head( channel_index, key_number ) = next_index
				else
					midi_notes( previous_index ).next_same_key = next_index
				end if
			else
				previous_index = note_index
			end if

			note_index = next_index
		wend
	next
end sub

sub MidiCloseChannelNotes( byval channel_index as integer, byval end_frame as longint )
	for key_number as integer = 0 to MIDI_KEY_COUNT - 1
		dim as integer note_index = midi_active_head( channel_index, key_number )
		while( note_index >= 0 )
			dim as integer next_index = midi_notes( note_index ).next_same_key
			midi_notes( note_index ).end_frame = end_frame
			if( midi_notes( note_index ).end_frame <= _
			    midi_notes( note_index ).start_frame ) then
				midi_notes( note_index ).end_frame = _
					midi_notes( note_index ).start_frame + 1
			end if
			note_index = next_index
		wend
		midi_active_head( channel_index, key_number ) = -1
	next
end sub

function MidiBuildNotes _
	( _
		byval division as integer, _
		byval sample_rate as integer, _
		byref song_end_frame as longint _
	) as integer

	midi_note_count = 0
	midi_note_capacity = 0
	midi_unmatched_note_offs = 0
	erase midi_notes

	for channel_index as integer = 0 to MIDI_CHANNEL_COUNT - 1
		midi_program( channel_index ) = 0
		midi_channel_volume( channel_index ) = 100
		midi_expression( channel_index ) = 127
		midi_pan( channel_index ) = 64
		midi_pitch_bend( channel_index ) = 8192
		midi_sustain( channel_index ) = 0

		for key_number as integer = 0 to MIDI_KEY_COUNT - 1
			midi_active_head( channel_index, key_number ) = -1
		next
	next

	dim as ulongint current_tick = 0
	dim as ulongint tempo_us = MIDI_DEFAULT_TEMPO_US
	dim as double exact_frame = 0.0
	dim as longint current_frame = 0

	for event_index as integer = 0 to midi_event_count - 1
		with midi_events( event_index )
			if( .tick > current_tick ) then
				dim as ulongint tick_delta = .tick - current_tick
				exact_frame += ( cdbl( tick_delta ) * cdbl( tempo_us ) * _
				                 cdbl( sample_rate ) ) / _
				               ( cdbl( division ) * 1000000.0 )
				current_frame = clngint( int( exact_frame + 0.5 ) )
				current_tick = .tick
			end if

			dim as integer channel_index = .channel
			select case .kind
			case MIDI_EVENT_TEMPO
				tempo_us = .tempo_us
			case MIDI_EVENT_PROGRAM
				midi_program( channel_index ) = .data1
			case MIDI_EVENT_PITCH
				midi_pitch_bend( channel_index ) = .data1 or ( cint( .data2 ) shl 7 )
			case MIDI_EVENT_CONTROL
				select case .data1
				case 7
					midi_channel_volume( channel_index ) = .data2
				case 10
					midi_pan( channel_index ) = .data2
				case 11
					midi_expression( channel_index ) = .data2
				case 64
					dim as integer new_sustain = iif( .data2 >= 64, 1, 0 )
					if( midi_sustain( channel_index ) <> 0 and new_sustain = 0 ) then
						MidiReleaseSustained( channel_index, current_frame )
					end if
					midi_sustain( channel_index ) = new_sustain
				case 120, 123
					MidiCloseChannelNotes( channel_index, current_frame )
				end select
			case MIDI_EVENT_NOTE_ON
				if( MidiStartNote( channel_index, .data1, .data2, current_frame ) <> 0 ) then
					return -1
				end if
			case MIDI_EVENT_NOTE_OFF
				MidiReleaseOneNote( channel_index, .data1, current_frame )
			end select
		end with
	next

	'' Malformed or deliberately open-ended tracks cannot leave voices active
	'' forever. Give such notes a half-second final gate after the last event.
	song_end_frame = current_frame
	dim as longint forced_end_frame = current_frame + sample_rate \ 2
	for channel_index as integer = 0 to MIDI_CHANNEL_COUNT - 1
		MidiCloseChannelNotes( channel_index, forced_end_frame )
	next

	for note_index as integer = 0 to midi_note_count - 1
		if( midi_notes( note_index ).end_frame < 0 ) then
			midi_notes( note_index ).end_frame = forced_end_frame
		end if

		if( note_index > 0 andalso _
		    midi_notes( note_index ).start_frame < _
		    midi_notes( note_index - 1 ).start_frame ) then
			MidiSetError( "internal MIDI note order is not monotonic" )
			return -1
		end if

		if( midi_notes( note_index ).end_frame > song_end_frame ) then
			song_end_frame = midi_notes( note_index ).end_frame
		end if
	next

	return 0
end function

'' -------------------------------------------------------------------------
'' Polyphony analysis
'' -------------------------------------------------------------------------

function MidiPeakPolyphony( byval render_end_frame as longint ) as integer
	if( midi_note_count <= 0 or render_end_frame <= 0 ) then return 0

	'' Notes are already ordered by start frame. A minimum heap of end frames
	'' tracks exactly which prior notes remain active at each new note onset.
	'' This avoids a second large event array for dense MIDI files.
	redim as longint end_heap( 0 to midi_note_count - 1 )
	dim as integer heap_count = 0
	dim as integer peak_count = 0

	for note_index as integer = 0 to midi_note_count - 1
		dim as longint start_frame = midi_notes( note_index ).start_frame
		if( start_frame >= render_end_frame ) then exit for

		dim as longint end_frame = midi_notes( note_index ).end_frame
		if( end_frame > render_end_frame ) then end_frame = render_end_frame
		if( end_frame <= start_frame ) then continue for

		while( heap_count > 0 andalso end_heap( 0 ) <= start_frame )
			heap_count -= 1
			if( heap_count > 0 ) then
				end_heap( 0 ) = end_heap( heap_count )
				dim as integer parent_index = 0

				do
					dim as integer left_index = parent_index * 2 + 1
					if( left_index >= heap_count ) then exit do

					dim as integer child_index = left_index
					dim as integer right_index = left_index + 1
					if( right_index < heap_count andalso _
					    end_heap( right_index ) < end_heap( left_index ) ) then
						child_index = right_index
					end if

					if( end_heap( parent_index ) <= end_heap( child_index ) ) then
						exit do
					end if

					swap end_heap( parent_index ), end_heap( child_index )
					parent_index = child_index
				loop
			end if
		wend

		dim as integer insert_index = heap_count
		heap_count += 1
		while( insert_index > 0 )
			dim as integer parent_index = ( insert_index - 1 ) \ 2
			if( end_heap( parent_index ) <= end_frame ) then exit while
			end_heap( insert_index ) = end_heap( parent_index )
			insert_index = parent_index
		wend
		end_heap( insert_index ) = end_frame

		if( heap_count > peak_count ) then peak_count = heap_count
	next

	return peak_count
end function

'' -------------------------------------------------------------------------
'' General MIDI synthesis mapping
'' -------------------------------------------------------------------------

sub MidiConfigureSynth()
	'' The sixteen General MIDI families each receive one inexpensive oscillator
	'' and envelope. These are intentionally synthetic timbres, not claims of
	'' sample-accurate piano, guitar, or orchestral emulation.
	dim as integer wave_type( 0 to 15 ) = _
		{ 2, 0, 1, 3, 1, 3, 3, 3, 1, 0, 1, 2, 3, 2, 1, 4 }
	dim as single attack( 0 to 15 ) = _
		{ .005, .002, .020, .005, .005, .080, .060, .025, _
		  .020, .030, .005, .120, .080, .010, .002, .001 }
	dim as single decay( 0 to 15 ) = _
		{ .180, .120, .080, .120, .080, .250, .220, .120, _
		  .120, .100, .080, .350, .300, .180, .100, .080 }
	dim as single sustain( 0 to 15 ) = _
		{ .35, .15, .80, .40, .50, .65, .60, .65, _
		  .55, .75, .65, .65, .40, .45, .20, .05 }
	dim as single release_time( 0 to 15 ) = _
		{ .12, .08, .12, .10, .08, .30, .35, .18, _
		  .15, .18, .12, .40, .30, .16, .08, .05 }

	for family_index as integer = 0 to 15
		wave family_index, wave_type( family_index )
		envelope family_index, attack( family_index ), decay( family_index ), _
		         sustain( family_index ), release_time( family_index )
		instrument family_index, family_index, family_index
	next
end sub

function MidiNoteFrequency( byref note_row as MidiNoteRow ) as integer
	if( note_row.channel = MIDI_DRUM_CHANNEL ) then
		select case note_row.key_number
		case 35, 36
			return 90
		case 38, 39, 40
			return 1800
		case 42, 44, 46
			return 7000
		case 49, 51, 52, 55, 57, 59
			return 12000
		case 41 to 47
			return 250 + ( note_row.key_number - 41 ) * 90
		case else
			return 3500
		end select
	end if

	dim as double bend_semitones = _
		( cdbl( note_row.pitch_bend ) - 8192.0 ) * ( 2.0 / 8192.0 )
	dim as double note_number = cdbl( note_row.key_number ) + bend_semitones
	dim as double frequency = 440.0 * ( 2.0 ^ ( ( note_number - 69.0 ) / 12.0 ) )

	if( frequency < 20.0 ) then frequency = 20.0
	if( frequency > 20000.0 ) then frequency = 20000.0
	return cint( frequency )
end function

function MidiDrumFrames _
	( _
		byval key_number as integer, _
		byval sample_rate as integer _
	) as longint

	dim as double seconds
	select case key_number
	case 35, 36
		seconds = .18
	case 38, 39, 40
		seconds = .16
	case 42, 44
		seconds = .07
	case 46
		seconds = .20
	case 49, 51, 52, 55, 57, 59
		seconds = .50
	case else
		seconds = .14
	end select

	return clngint( seconds * sample_rate )
end function

sub MidiAdvanceFrames( byval frame_count as longint )
	while( frame_count > 0 )
		dim as integer step_frames
		if( frame_count > 32768 ) then
			step_frames = 32768
		else
			step_frames = cint( frame_count )
		end if

		fb_sfxUpdate( step_frames )
		frame_count -= step_frames
	wend
end sub

function MidiRenderSong _
	( _
		byval sample_rate as integer, _
		byval render_end_frame as longint, _
		byval declared_peak as integer, _
		byref started_notes as integer, _
		byref dropped_notes as integer, _
		byref muted_notes as integer, _
		byref actual_peak as integer _
	) as integer

	started_notes = 0
	dropped_notes = 0
	muted_notes = 0
	actual_peak = 0

	dim as integer normalized_polyphony = declared_peak
	if( normalized_polyphony < 1 ) then normalized_polyphony = 1
	if( normalized_polyphony > 64 ) then normalized_polyphony = 64
	dim as single master_gain = 0.36 / sqr( csng( normalized_polyphony ) )

	dim as longint rendered_frame = 0
	dim as integer note_index = 0
	while( note_index < midi_note_count )
		dim as longint start_frame = midi_notes( note_index ).start_frame
		if( start_frame >= render_end_frame ) then exit while

		if( start_frame > rendered_frame ) then
			MidiAdvanceFrames( start_frame - rendered_frame )
			rendered_frame = start_frame
		end if

		while( note_index < midi_note_count andalso _
		       midi_notes( note_index ).start_frame = start_frame )
			with midi_notes( note_index )
				dim as longint end_frame = .end_frame
				if( end_frame > render_end_frame ) then end_frame = render_end_frame

				if( .channel = MIDI_DRUM_CHANNEL ) then
					dim as longint drum_end = start_frame + _
						MidiDrumFrames( .key_number, sample_rate )
					if( end_frame > drum_end ) then end_frame = drum_end
				end if

				if( end_frame > start_frame ) then
					dim as longint duration_frames = end_frame - start_frame

					'' SOUND accepts seconds as a Single. Subtract one frame before
					'' conversion so floating-point rounding cannot leave the old
					'' chord alive for one frame when the next chord begins.
					if( duration_frames > 1 ) then duration_frames -= 1

					dim as double velocity_scale = _
						sqr( cdbl( .velocity ) / 127.0 )
					dim as double controller_scale = _
						( cdbl( .channel_volume ) / 127.0 ) * _
						( cdbl( .expression ) / 127.0 )
					dim as single voice_volume = _
						csng( master_gain * velocity_scale * controller_scale )

					if( voice_volume > 0.0001 ) then
						dim as integer instrument_family = .program_number \ 8
						if( .channel = MIDI_DRUM_CHANNEL ) then instrument_family = 15

						dim as single pan_position = _
							( csng( .pan_value ) - 64.0 ) / 63.0
						if( pan_position < -1.0 ) then pan_position = -1.0
						if( pan_position > 1.0 ) then pan_position = 1.0

						instrument .channel, instrument_family
						pan .channel, pan_position

						dim as integer before_count = fb_sfxVoiceActiveCount()
						sound .channel, MidiNoteFrequency( midi_notes( note_index ) ), _
						      csng( cdbl( duration_frames ) / sample_rate ), _
						      voice_volume
						dim as integer after_count = fb_sfxVoiceActiveCount()

						if( after_count > before_count ) then
							started_notes += 1
						else
							dropped_notes += 1
						end if

						if( after_count > actual_peak ) then actual_peak = after_count
					else
						muted_notes += 1
					end if
				end if
			end with

			note_index += 1
		wend
	wend

	if( render_end_frame > rendered_frame ) then
		MidiAdvanceFrames( render_end_frame - rendered_frame )
	end if

	return 0
end function

sub MidiPlayerShutdown _
	( _
		byref capture_active as integer, _
		byref foreground_feed_active as integer _
	)

	if( capture_active <> 0 ) then
		sfxlib.OutputCaptureStop()
		capture_active = 0
	end if

	sfxlib.EchoReset()

	if( foreground_feed_active <> 0 ) then
		fb_sfxForegroundFeedEnd()
		foreground_feed_active = 0
	end if
end sub

'' -------------------------------------------------------------------------
'' Program entry point
'' -------------------------------------------------------------------------

dim as string midi_filename = trim( command( 1 ) )
dim as string capture_filename = trim( command( 2 ) )
dim as double maximum_seconds = val( command( 3 ) )
dim as integer echo_enabled = iif( lcase( trim( command( 4 ) ) ) = "noecho", 0, 1 )

if( midi_filename = "" ) then
	midi_filename = exepath() + "\media\harmonized-scale.mid"
end if

if( capture_filename = "" ) then
	capture_filename = exepath() + "\midi-synth-output.wav"
elseif( capture_filename = "-" ) then
	capture_filename = ""
end if

if( maximum_seconds < 0.0 ) then
	print "ERROR: maximum seconds cannot be negative"
	end 1
end if

dim as integer format_number
dim as integer track_count
dim as integer division

print
print "SFXLIB SOFTWARE-SYNTH MIDI PLAYER"
print "MIDI file: "; midi_filename

if( MidiLoadFile( midi_filename, format_number, track_count, division ) <> 0 ) then
	print "ERROR: "; midi_error_text
	end 1
end if

if( midi_event_count <= 0 ) then
	print "ERROR: MIDI file contains no playable or timing events"
	end 1
end if

MidiSortEvents( 0, midi_event_count - 1 )

dim as integer foreground_feed_active = 0
dim as integer capture_active = 0
dim as integer result_code = 0

fb_sfxForegroundFeedBegin()
foreground_feed_active = 1
MidiConfigureSynth()

dim as integer sample_rate = sfxlib.OutputSampleRate()
if( sample_rate <= 0 ) then
	print "ERROR: sfxlib could not initialize an output driver"
	MidiPlayerShutdown( capture_active, foreground_feed_active )
	end 1
end if

dim as longint song_end_frame
if( MidiBuildNotes( division, sample_rate, song_end_frame ) <> 0 ) then
	print "ERROR: "; midi_error_text
	MidiPlayerShutdown( capture_active, foreground_feed_active )
	end 1
end if

if( midi_note_count <= 0 ) then
	print "ERROR: MIDI file contains no note events"
	MidiPlayerShutdown( capture_active, foreground_feed_active )
	end 1
end if

dim as longint render_end_frame = song_end_frame
if( maximum_seconds > 0.0 ) then
	dim as longint requested_end = clngint( maximum_seconds * sample_rate )
	if( requested_end < render_end_frame ) then render_end_frame = requested_end
end if

if( render_end_frame <= 0 ) then
	print "ERROR: selected MIDI playback interval is empty"
	MidiPlayerShutdown( capture_active, foreground_feed_active )
	end 1
end if

dim as double render_seconds = cdbl( render_end_frame ) / sample_rate
dim as longint render_milliseconds = _
	clngint( int( render_seconds * 1000.0 + 0.5 ) )
if( capture_filename <> "" and render_seconds > MIDI_MAX_CAPTURE_SECONDS ) then
	print "ERROR: WAV capture is limited to "; MIDI_MAX_CAPTURE_SECONDS; " seconds"
	print "       Pass a maximum-seconds argument or use '-' as the WAV filename."
	MidiPlayerShutdown( capture_active, foreground_feed_active )
	end 1
end if

dim as integer declared_peak = MidiPeakPolyphony( render_end_frame )

print "Format:"; format_number; "  tracks:"; track_count; _
      "  ticks/quarter:"; division
print "Parsed events:"; midi_event_count; "  notes:"; midi_note_count
print "Duration: "; render_milliseconds \ 1000; "."; _
      right( "000" + ltrim( str( render_milliseconds mod 1000 ) ), 3 ); _
      " seconds at"; sample_rate; "Hz"
print "Peak MIDI polyphony:"; declared_peak

if( midi_unmatched_note_offs > 0 ) then
	print "Unmatched note-off events:"; midi_unmatched_note_offs
end if

if( echo_enabled <> 0 ) then
	if( sfxlib.Echo( 0.12, 0.14, 0.24 ) <> 0 ) then
		print "ERROR: could not enable the echo effect"
		MidiPlayerShutdown( capture_active, foreground_feed_active )
		end 1
	end if
end if

if( capture_filename <> "" ) then
	if( sfxlib.OutputCaptureStart() <> 0 ) then
		print "ERROR: could not start the output capture"
		MidiPlayerShutdown( capture_active, foreground_feed_active )
		end 1
	end if
	capture_active = 1

	dim as longint reserve_frames = render_end_frame
	if( echo_enabled <> 0 ) then reserve_frames += sample_rate \ 2

	if( reserve_frames <= 2147483647 ) then
		if( sfxlib.OutputCaptureReserve( clng( reserve_frames ) ) <> 0 ) then
			print "ERROR: could not reserve the output capture"
			MidiPlayerShutdown( capture_active, foreground_feed_active )
			end 1
		end if
	end if
end if

dim as ulongint underruns_before = sfxlib.OutputUnderruns()
dim as double started_at = timer
dim as integer started_notes
dim as integer dropped_notes
dim as integer muted_notes
dim as integer actual_peak

if( MidiRenderSong( sample_rate, render_end_frame, declared_peak, _
                    started_notes, dropped_notes, muted_notes, _
                    actual_peak ) <> 0 ) then
	print "ERROR: MIDI render failed"
	MidiPlayerShutdown( capture_active, foreground_feed_active )
	end 1
end if

'' Let the feedback delay decay after the last scheduled note. This also makes
'' the captured output prove that the optional effect remained active.
if( echo_enabled <> 0 ) then MidiAdvanceFrames( sample_rate \ 2 )

dim as double elapsed_seconds = timer - started_at
if( elapsed_seconds < 0.0 ) then elapsed_seconds += 86400.0
dim as ulongint underruns_after = sfxlib.OutputUnderruns()
dim as ulongint underrun_count = underruns_after - underruns_before

if( capture_active <> 0 ) then
	sfxlib.OutputCaptureStop()
	capture_active = 0

	if( sfxlib.OutputCaptureSave( strptr( capture_filename ) ) <> 0 ) then
		print "ERROR: could not save output capture"
		MidiPlayerShutdown( capture_active, foreground_feed_active )
		end 1
	end if
end if

print "Started synth notes:"; started_notes
print "Muted notes skipped:"; muted_notes
print "Peak allocated voices:"; actual_peak
print "Dropped notes:"; dropped_notes
print "Driver underruns:"; underrun_count

if( capture_filename <> "" ) then print "Captured WAV: "; capture_filename
if( lcase( trim( environ( "SFXLIB_DRIVER" ) ) ) = "null" and _
    elapsed_seconds > 0.0 ) then
	print "Offline render speed: ";
	dim as double render_factor = _
		( render_seconds + iif( echo_enabled <> 0, 0.5, 0.0 ) ) / _
		elapsed_seconds
	print int( render_factor * 10.0 + 0.5 ) / 10.0; "x real time"
end if

if( dropped_notes <> 0 ) then
	print "ERROR: the 64-voice pool could not allocate every MIDI note"
	result_code = 2
end if

if( underrun_count <> 0 ) then
	print "ERROR: the output driver starved during MIDI playback"
	result_code = 3
end if

if( result_code = 0 ) then
	print "PASS: MIDI was synthesized entirely through sfxlib"
end if

MidiPlayerShutdown( capture_active, foreground_feed_active )

end result_code

'' end of midi-synth-player.bas
