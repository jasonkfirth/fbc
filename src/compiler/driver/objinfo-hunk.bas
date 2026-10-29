'' Project: FreeBASIC compiler object metadata
'' File: driver/objinfo-hunk.bas
'' Purpose: Locate .fbctinf in a relocatable Amiga Hunk object.
'' Responsibilities: Decode big-endian records and bound every count and offset.
'' This file intentionally does NOT contain: input ownership, executable loading, or linking.

#include once "driver/objinfo-hunk.bi"

'' Hunk tags and external entry kinds follow the NDK dos/doshunks.h contract.
'' Every scalar and length is big-endian, independently of the compiler host.
private function hReadLong _
	( byval buffer as const ubyte ptr, byval byte_count as integer, _
	  byref position as integer, byref value as ulong ) as integer
	if( (position < 0) or (byte_count < 4) ) then return FALSE
	if( position > byte_count - 4 ) then return FALSE
	value = (culng(buffer[position]) shl 24) or (culng(buffer[position + 1]) shl 16) or _
		(culng(buffer[position + 2]) shl 8) or culng(buffer[position + 3])
	position += 4
	return TRUE
end function

private function hSkipLongs _
	( byval byte_count as integer, byref position as integer, byval count as ulong ) as integer
	if( (position < 0) or (position > byte_count) ) then return FALSE
	if( culngint(count) > culngint((byte_count - position) \ 4) ) then return FALSE
	position += cint(count) * 4
	return TRUE
end function

private function hSkipSymbols _
	( byval buffer as const ubyte ptr, byval byte_count as integer, _
	  byref position as integer, byval external_entries as integer ) as integer
	dim as ulong descriptor, entry_kind, count, unused
	do
		if( hReadLong(buffer, byte_count, position, descriptor) = FALSE ) then return FALSE
		if( descriptor = 0 ) then return TRUE
		entry_kind = descriptor shr 24
		if( external_entries = FALSE ) then
			if( entry_kind <> 0 ) then return FALSE
		end if
		if( hSkipLongs(byte_count, position, descriptor and &hfffffful) = FALSE ) then return FALSE
		select case entry_kind
		case 0, 1, 2, 3 '' symbol, relocatable/absolute definition, resident
			if( hSkipLongs(byte_count, position, 1) = FALSE ) then return FALSE
		case 130, 137 '' common symbols include their size before the references
			if( hReadLong(buffer, byte_count, position, unused) = FALSE ) then return FALSE
			if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
			if( hSkipLongs(byte_count, position, count) = FALSE ) then return FALSE
		case 129, 131 to 136, 138, 139
			if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
			if( hSkipLongs(byte_count, position, count) = FALSE ) then return FALSE
		case else
			return FALSE
		end select
	loop
end function

private function hSkipShortRelocations _
	( byval buffer as const ubyte ptr, byval byte_count as integer, _
	  byref position as integer ) as integer
	dim as integer count
	do
		if( position > byte_count - 2 ) then return FALSE
		count = (cuint(buffer[position]) shl 8) or buffer[position + 1]
		position += 2
		if( count = 0 ) then exit do
		'' One target hunk plus COUNT word-sized relocation offsets.
		if( count + 1 > (byte_count - position) \ 2 ) then return FALSE
		position += (count + 1) * 2
	loop
	if( (position and 3) <> 0 ) then
		if( position > byte_count - 2 ) then return FALSE
		position += 2
	end if
	return TRUE
end function

function objinfoLoadHunk _
	( byval buffer as const ubyte ptr, byval byte_count as integer, _
	  byref section_data as const ubyte ptr, byref section_size as integer ) as integer
	section_data = 0
	section_size = 0
	if( (buffer = 0) or (byte_count < 8) ) then return FALSE
	dim as integer position = 0
	dim as ulong tag, count, unused
	if( hReadLong(buffer, byte_count, position, tag) = FALSE ) then return FALSE
	if( tag <> 999 ) then return FALSE '' HUNK_UNIT: reject load modules and raw data
	if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
	if( hSkipLongs(byte_count, position, count) = FALSE ) then return FALSE
	dim as integer metadata_name = FALSE
	dim as integer metadata_start = -1, metadata_bytes = 0
	do while( position < byte_count )
		if( hReadLong(buffer, byte_count, position, tag) = FALSE ) then return FALSE
		'' Chip/fast placement flags do not change the record payload.
		tag and= &h3ffffffful
		select case tag
		case 1000 '' HUNK_NAME, padded to a whole number of longwords
			if( metadata_start >= 0 ) then return FALSE
			if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
			dim as integer name_start = position
			if( hSkipLongs(byte_count, position, count) = FALSE ) then return FALSE
			metadata_name = FALSE
			if( count >= 2 ) then
				metadata_name = TRUE
				dim as zstring * 9 expected = ".fbctinf"
				for index as integer = 0 to position - name_start - 1
					dim as integer wanted = 0
					if( index < 8 ) then wanted = asc(expected, index + 1)
					if( buffer[name_start + index] <> wanted ) then
						metadata_name = FALSE
						exit for
					end if
				next
			end if
		case 1001, 1002 '' HUNK_CODE, HUNK_DATA
			if( metadata_start >= 0 ) then return FALSE
			if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
			dim as integer data_start = position
			if( hSkipLongs(byte_count, position, count) = FALSE ) then return FALSE
			if( metadata_name and (tag = 1002) and (count > 0) ) then
				metadata_start = data_start
				metadata_bytes = position - data_start
			end if
			metadata_name = FALSE
		case 1003 '' HUNK_BSS reserves memory but has no bytes in the object
			if( metadata_start >= 0 ) then return FALSE
			if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
			metadata_name = FALSE
		case 1004 to 1006, 1015 to 1017, 1021, 1022
			'' Relocations are count/target/offset groups, terminated by zero.
			do
				if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
				if( count = 0 ) then exit do
				if( hReadLong(buffer, byte_count, position, unused) = FALSE ) then return FALSE
				if( hSkipLongs(byte_count, position, count) = FALSE ) then return FALSE
			loop
		case 1007, 1008 '' HUNK_EXT, HUNK_SYMBOL
			if( hSkipSymbols(buffer, byte_count, position, tag = 1007) = FALSE ) then return FALSE
		case 1009 '' HUNK_DEBUG carries a longword count and opaque payload
			if( hReadLong(buffer, byte_count, position, count) = FALSE ) then return FALSE
			if( hSkipLongs(byte_count, position, count) = FALSE ) then return FALSE
		case 1010 '' HUNK_END closes the current section
			if( metadata_start >= 0 ) then
				section_data = buffer + metadata_start
				section_size = metadata_bytes
				return TRUE
			end if
			metadata_name = FALSE
		case 1020 '' HUNK_RELOC32SHORT uses 16-bit groups and longword padding
			if( hSkipShortRelocations(buffer, byte_count, position) = FALSE ) then return FALSE
		case else
			'' Unsupported records cannot be searched as arbitrary byte strings.
			return FALSE
		end select
	loop
	return FALSE
end function

'' end of driver/objinfo-hunk.bas
