'' Project: FreeBASIC compiler object metadata
'' File: driver/objinfo-hunk.bi
'' Purpose: Declare the bounded Amiga Hunk section reader.
'' Responsibilities: Return a borrowed metadata span without changing the input.
'' This file intentionally does NOT contain: archive handling or linker policy.

declare function objinfoLoadHunk _
	( byval buffer as const ubyte ptr, byval byte_count as integer, _
	  byref section_data as const ubyte ptr, byref section_size as integer ) as integer

'' end of driver/objinfo-hunk.bi
