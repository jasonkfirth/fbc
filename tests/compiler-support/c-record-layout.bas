'' Project: FreeBASIC C backend tests
'' File: c-record-layout.bas
'' Purpose: Exercise named-field offsets where whole-record size can conceal a mismatch.
'' Responsibilities: Cover ordinary, anonymous, packed, array and fixed-text members.
'' This file intentionally does NOT change target ABI declarations or read padding bytes.
#lang "fb"

type LayoutRecord
	first as ubyte
	middle as ushort
	last as ubyte
end type

type LayoutAnonymous
	tag as ubyte
	union
		type
			left_word as ushort
			right_byte as ubyte
		end type
		whole as uinteger
	end union
	tail as ubyte
end type

type LayoutPacked field = 2
	first as ubyte
	middle as uinteger
	last as ubyte
end type

type LayoutArrays
	values(2 to 4) as LayoutRecord
	label_text as string * 7
end type

dim ordinary as LayoutRecord
dim anonymous_record as LayoutAnonymous
dim packed_record as LayoutPacked
dim array_record as LayoutArrays

ordinary.middle = 127
anonymous_record.left_word = 17
anonymous_record.right_byte = 9
anonymous_record.tail = 3
'' A non-symmetric pattern observes packed native-width field storage.
packed_record.middle = &h12345678u
packed_record.last = 5
array_record.values(3).middle = 129
array_record.label_text = "abcdefg"

if ordinary.middle <> 127 then end 1
if anonymous_record.left_word <> 17 then end 2
if anonymous_record.right_byte <> 9 then end 3
if anonymous_record.tail <> 3 then end 4
if packed_record.middle <> &h12345678u then end 5
if packed_record.last <> 5 then end 6
if array_record.values(3).middle <> 129 then end 7
if array_record.label_text <> "abcdefg" then end 8
if sizeof(LayoutRecord) <> 6 then end 9
print "RECORD_LAYOUT_PASS"; sizeof(LayoutRecord)

'' end of c-record-layout.bas
