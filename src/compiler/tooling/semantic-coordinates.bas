'' Project: FreeBASIC compiler - semantic source coordinates
'' File: tooling/semantic-coordinates.bas
'' Purpose: Link physical lexer positions to bytes from the opened revision.
'' Responsibilities: Decode coordinate boundaries and serialize proven spans.
'' This file intentionally does NOT contain: language parsing or macro expansion.

#include once "tooling/semantic-private.bi"
#include once "tooling/semantic-source.bi"
#include once "tooling/semantic-coordinates.bi"
#include once "file.bi"

'' -------------------------------------------------------------------------
'' Per-source line cache
'' -------------------------------------------------------------------------

'' Source modules compile serially. This cache reads the compiler's binary
'' handle and restores its position; it never substitutes a pathname reopen.
'' A source occurrence ID distinguishes repeated includes and #line aliases.
const SOURCE_COORDINATE_CHUNK_BYTES = 8192
const SOURCE_COORDINATE_MAX_LINE_BYTES = 16777216
'' Retain at most 8 MiB of line starts. Larger files still use the forward
'' reader, so reaching this cache limit does not discard coordinate facts.
const SOURCE_COORDINATE_INDEX_LINES = 1048576
dim shared as longint coordinate_index_source
dim shared as longint coordinate_line_offsets( )
dim shared as integer coordinate_index_lines
dim shared as longint coordinate_source
dim shared as integer coordinate_line, coordinate_next_line
dim shared as longint coordinate_offset, coordinate_next_offset
dim shared as string coordinate_text
dim shared as integer coordinate_format

sub fbSemanticModelResetCoordinates( )
	coordinate_index_source = 0
	coordinate_index_lines = 0
	erase coordinate_line_offsets
	coordinate_source = 0
	coordinate_line = 0
	coordinate_next_line = 1
	coordinate_offset = 0
	coordinate_next_offset = 0
	coordinate_text = ""
end sub

private function hUnitWidth( ) as integer
	select case coordinate_format
	case FBFILE_FORMAT_UTF16LE, FBFILE_FORMAT_UTF16BE: return 2
	case FBFILE_FORMAT_UTF32LE, FBFILE_FORMAT_UTF32BE: return 4
	case else: return 1
	end select
end function

private function hBOMBytes( ) as integer
	select case coordinate_format
	case FBFILE_FORMAT_UTF8: return 3
	case FBFILE_FORMAT_UTF16LE, FBFILE_FORMAT_UTF16BE: return 2
	case FBFILE_FORMAT_UTF32LE, FBFILE_FORMAT_UTF32BE: return 4
	case else: return 0
	end select
end function

private function hCodeUnit( byref value as const string, byval offset as integer ) as ulong
	dim as ulong result = 0
	dim as integer unit_width = hUnitWidth( )
	if( (coordinate_format = FBFILE_FORMAT_UTF16BE) or (coordinate_format = FBFILE_FORMAT_UTF32BE) ) then
		for index as integer = 0 to unit_width - 1
			result = (result shl 8) or value[offset + index]
		next
	else
		for index as integer = 0 to unit_width - 1
			result or= culng(value[offset + index]) shl (index * 8)
		next
	end if
	return result
end function

private sub hRememberLineStart( byval line_number as integer, byval offset as longint )
	if( (line_number <> coordinate_index_lines + 1) or _
	    (coordinate_index_lines >= SOURCE_COORDINATE_INDEX_LINES) ) then exit sub
	if( coordinate_index_lines > ubound(coordinate_line_offsets) ) then
		dim as integer capacity = (ubound(coordinate_line_offsets) + 1) * 2
		if( capacity > SOURCE_COORDINATE_INDEX_LINES ) then capacity = SOURCE_COORDINATE_INDEX_LINES
		redim preserve coordinate_line_offsets(0 to capacity - 1)
	end if
	coordinate_line_offsets(coordinate_index_lines) = offset
	coordinate_index_lines += 1
end sub

private function hReadLine( byval source as longint, byval line_number as integer ) as integer
	if( (source = 0) or (line_number < 1) ) then return FALSE
	dim as integer source_handle = fbSemanticModelSourceHandle(source, coordinate_format)
	if( source_handle = 0 ) then return FALSE
	if( (source = coordinate_source) and (line_number = coordinate_line) ) then return TRUE
	dim as longint previous_position = seek(source_handle), bytes = lof(source_handle)
	dim as integer unit_width = hUnitWidth( ), current_line = 1
	dim as longint position = hBOMBytes( )
	'' Expression and block ranges revisit earlier lines after their children.
	'' Remember starts already crossed in this opened source occurrence instead
	'' of rescanning the whole preprocessed translation unit on every revisit.
	if( source <> coordinate_index_source ) then
		coordinate_index_source = source
		coordinate_index_lines = 1
		redim coordinate_line_offsets(0 to 255)
		coordinate_line_offsets(0) = position
	end if
	if( line_number <= coordinate_index_lines ) then
		current_line = line_number
	else
		current_line = coordinate_index_lines
	end if
	position = coordinate_line_offsets(current_line - 1)
	if( (source = coordinate_source) and (line_number >= coordinate_next_line) ) then
		if( coordinate_next_line > current_line ) then
			current_line = coordinate_next_line
			position = coordinate_next_offset
		end if
	end if
	dim as integer ok = TRUE, found = FALSE
	dim as string text
	dim as longint line_start = position
	while( ok and (current_line <= line_number) )
		line_start = position
		text = ""
		dim as integer terminated = FALSE
		while( (position < bytes) and (terminated = FALSE) and ok )
			dim as integer count = SOURCE_COORDINATE_CHUNK_BYTES
			if( bytes - position < count ) then count = bytes - position
			count -= count mod unit_width
			if( count = 0 ) then
				ok = FALSE
				exit while
			end if
			dim as string chunk = space(count)
			if( get(#source_handle, position + 1, chunk) <> 0 ) then
				ok = FALSE
				exit while
			end if
			dim as integer taken = count
			for index as integer = 0 to count - unit_width step unit_width
				dim as ulong unit = hCodeUnit(chunk, index)
				if( (unit = CHAR_CR) or (unit = CHAR_LF) ) then
					taken = index
					position += index + unit_width
					if( (unit = CHAR_CR) and (position + unit_width <= bytes) ) then
						dim as string next_unit = space(unit_width)
						if( get(#source_handle, position + 1, next_unit) <> 0 ) then
							ok = FALSE
						elseif( hCodeUnit(next_unit, 0) = CHAR_LF ) then
							position += unit_width
						end if
					end if
					terminated = TRUE
					exit for
				end if
				next
			if( current_line = line_number ) then
				if( len(text) > SOURCE_COORDINATE_MAX_LINE_BYTES - taken ) then
					ok = FALSE
				else
					text += left(chunk, taken)
				end if
			end if
			if( terminated = FALSE ) then position += count
		wend
		if( ok and terminated ) then hRememberLineStart(current_line + 1, position)
		if( current_line = line_number ) then
			found = ok
			exit while
		end if
		if( terminated = FALSE ) then exit while
		current_line += 1
	wend
	seek #source_handle, previous_position
	if( found = FALSE ) then return FALSE
	coordinate_source = source
	coordinate_line = line_number
	coordinate_offset = line_start
	coordinate_text = text
	coordinate_next_line = line_number + 1
	coordinate_next_offset = position
	return TRUE
end function

'' -------------------------------------------------------------------------
'' UTF-16 coordinate and byte-boundary projection
'' -------------------------------------------------------------------------

private function hResolveColumn _
	( byval source as longint, byval line_number as integer, byval decoder_column as integer, _
	  byref column as integer, byref offset as longint ) as integer
	if( (decoder_column < 0) or (hReadLine(source, line_number) = FALSE) ) then return FALSE
	dim as integer index = 0, decoded = 0, canonical = 0, selected = -1, selected_column = -1
	dim as integer unit_width = hUnitWidth( )
	while( index < len(coordinate_text) )
		if( decoded = decoder_column ) then
			selected = index
			selected_column = canonical
		end if
		dim as ulong codepoint = hCodeUnit(coordinate_text, index)
		dim as integer units = unit_width, columns = 1
		if( unit_width = 1 ) then
			if( codepoint > &h7F ) then
				dim as integer trailing = 0
				select case codepoint
				case &hC2 to &hDF: trailing = 1
				case &hE0 to &hEF: trailing = 2
				case &hF0 to &hF4: trailing = 3
				case else: return FALSE
				end select
				if( index + trailing >= len(coordinate_text) ) then return FALSE
				for extra as integer = 1 to trailing
					dim as integer next_byte = coordinate_text[index + extra]
					if( (next_byte < &h80) or (next_byte > &hBF) ) then return FALSE
					if( extra = 1 ) then
						if( (codepoint = &hE0) and (next_byte < &hA0) ) then return FALSE
						if( (codepoint = &hED) and (next_byte > &h9F) ) then return FALSE
						if( (codepoint = &hF0) and (next_byte < &h90) ) then return FALSE
						if( (codepoint = &hF4) and (next_byte > &h8F) ) then return FALSE
					end if
					next
				units = trailing + 1
				columns = iif(trailing = 3, 2, 1)
			end if
		elseif( unit_width = 2 ) then
			if( (codepoint >= &hD800) and (codepoint <= &hDBFF) ) then
				if( index + 3 >= len(coordinate_text) ) then return FALSE
				dim as ulong low = hCodeUnit(coordinate_text, index + 2)
				if( (low < &hDC00) or (low > &hDFFF) ) then return FALSE
				units = 4
				columns = 2
			elseif( (codepoint >= &hDC00) and (codepoint <= &hDFFF) ) then
				return FALSE
			end if
		else
			if( (codepoint > &h10FFFF) or ((codepoint >= &hD800) and (codepoint <= &hDFFF)) ) then return FALSE
			columns = iif(codepoint > &hFFFF, 2, 1)
		end if
		dim as integer decoder_columns = columns
		'' A byte-sized host wide buffer substitutes a single character for a
		'' decoded scalar it cannot represent. Keep that decoder coordinate
		'' separate from the UTF-16 position in the original encoded revision.
		if( (coordinate_format <> FBFILE_FORMAT_ASCII) and (sizeof(wstring) = 1) ) then decoder_columns = 1
		decoded += decoder_columns
		canonical += columns
		index += units
	wend
	if( decoded = decoder_column ) then
		selected = index
		selected_column = canonical
	end if
	if( selected < 0 ) then return FALSE
	column = selected_column
	offset = coordinate_offset + selected
	return TRUE
end function

function fbSemanticModelCoordinateFact _
	( byref first as LEX_LOCATION, byref last as LEX_LOCATION ) as string
	if( fbSemanticModelEnabled( ) = FALSE ) then return ""
	if( (first.raw_valid = FALSE) or (last.raw_valid = FALSE) or (first.source_context = 0) or _
	    (first.source_context <> last.source_context) ) then return ""
	'' Empty colon statements can begin after the parser's last consumed token.
	'' Do not publish an editable location when its endpoint precedes its start.
	if( first.raw_start_line > last.raw_end_line ) then return ""
	if( (first.raw_start_line = last.raw_end_line) and _
	    (first.raw_start_column > last.raw_end_column) ) then return ""
	dim as integer start_column = 0, end_column = 0
	dim as longint start_byte = -1, end_byte = -1
	dim as integer mapped = hResolveColumn(first.source_context, first.raw_start_line, first.raw_start_column, start_column, start_byte)
	mapped and= hResolveColumn(last.source_context, last.raw_end_line, last.raw_end_column, end_column, end_byte)
	if( mapped = FALSE ) then
		start_column = first.raw_start_column
		end_column = last.raw_end_column
		start_byte = -1
		end_byte = -1
	end if
	return fbSemanticModelNumber(first.source_context) + TABCHAR + fbSemanticModelNumber(first.raw_start_line) + _
		TABCHAR + fbSemanticModelNumber(start_column) + TABCHAR + fbSemanticModelNumber(last.raw_end_line) + _
		TABCHAR + fbSemanticModelNumber(end_column) + TABCHAR + fbSemanticModelNumber(start_byte) + _
		TABCHAR + fbSemanticModelNumber(end_byte) + TABCHAR + iif(mapped, "mapped", "unverified")
end function

sub fbSemanticModelExportCoordinates _
	( byref domain as const string, byval subject as longint, byref role as const string, _
	  byref first as LEX_LOCATION, byref last as LEX_LOCATION )
	dim as string coordinates = fbSemanticModelCoordinateFact(first, last)
	if( len(coordinates) = 0 ) then exit sub
	fbSemanticModelAppendProvenance("LOC" + TABCHAR + domain + TABCHAR + fbSemanticModelNumber(subject) + _
		TABCHAR + role + TABCHAR + coordinates)
end sub

'' end of tooling/semantic-coordinates.bas
