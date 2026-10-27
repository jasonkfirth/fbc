'' TEST_MODE : COMPILE_AND_RUN_OK
'' Project: FreeBASIC compiler regression tests
'' File: c-alignment-pointer-fields.bas
'' Purpose: Preserve pointer qualifiers through ordinary and packed field access.
'' Responsibilities: Cover text pointers, nested pointer levels and raw fixed
''     strings without assuming that the containing record is aligned.
'' This file intentionally does NOT write through readonly text pointers.
#lang "fb"

type pointer_fields
	readonly_text as const zstring ptr
	writable_text as zstring ptr
	readonly_wide as const wstring ptr
	raw_text as zstring * 12
	raw_wide as wstring * 12
end type
'' Sentinels expose pointer copies extending beyond their packed fields.
type packed_pointer_fields field = 1
	marker as ubyte
	readonly_text as const zstring ptr
	readonly_wide as const wstring ptr
	indirect_text as const zstring ptr ptr
	tail as ubyte
end type

dim shared as integer address_calls
private function record_address( byval bytes as ubyte ptr ) as pointer_fields ptr
	address_calls += 1
	return cptr(pointer_fields ptr, bytes + 1)
end function

dim text as zstring * 16 = "original"
dim wide_text as wstring * 16 = wstr("wide")
dim as const zstring ptr text_handle = @text
dim bytes(0 to 255) as ubyte
record_address(@bytes(0))->readonly_text = @text
if( address_calls <> 1 ) then end 1
dim as pointer_fields ptr record_pointer = cptr(pointer_fields ptr, @bytes(1))
record_pointer->writable_text = @text
record_pointer->readonly_wide = @wide_text
record_pointer->raw_text = "fixed"
record_pointer->raw_wide = wstr("wide-fixed")
dim as const pointer_fields ptr readonly_record = record_pointer
if( *readonly_record->readonly_text <> "original" ) then end 2
if( *readonly_record->readonly_wide <> wstr("wide") ) then end 3
if( readonly_record->raw_text <> "fixed" ) then end 4
if( readonly_record->raw_wide <> wstr("wide-fixed") ) then end 5
if( readonly_record->raw_wide >= wstr("wide-fixef") ) then end 14
if( readonly_record->raw_wide <= wstr("wide-fixec") ) then end 15
record_pointer->raw_wide = wstr("")
if( readonly_record->raw_wide <> wstr("") ) then end 16
*record_pointer->writable_text = "changed"
if( *readonly_record->readonly_text <> "changed" ) then end 6
record_pointer->readonly_text = 0
if( readonly_record->readonly_text <> 0 ) then end 7
'' Use separate buffer halves; offset one keeps both record views unaligned.
dim as packed_pointer_fields ptr packed_pointer = cptr(packed_pointer_fields ptr, @bytes(129))
packed_pointer->marker = &h6a
packed_pointer->tail = &ha6
packed_pointer->readonly_text = @text
packed_pointer->readonly_wide = @wide_text
packed_pointer->indirect_text = @text_handle
if( *packed_pointer->readonly_text <> "changed" ) then end 8
if( *packed_pointer->readonly_wide <> wstr("wide") ) then end 9
if( **packed_pointer->indirect_text <> "changed" ) then end 10
packed_pointer->readonly_text = 0
packed_pointer->readonly_wide = 0
packed_pointer->indirect_text = 0
if( packed_pointer->readonly_text <> 0 or packed_pointer->readonly_wide <> 0 ) then end 11
if( packed_pointer->indirect_text <> 0 ) then end 12
if( packed_pointer->marker <> &h6a or packed_pointer->tail <> &ha6 ) then end 13
print "c-alignment-pointer-fields-ok"
'' end of c-alignment-pointer-fields.bas
