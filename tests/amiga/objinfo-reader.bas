'' Project: FreeBASIC Amiga object metadata qualification
'' File: objinfo-reader.bas
'' Purpose: Check Hunk section discovery without trusting object offsets.
'' Responsibilities: Positive record forms, truncated inputs and oversized counts.
'' This file intentionally does NOT contain a linker or byte-search fallback.

#include once "driver/objinfo-hunk.bi"

private function LongWord(byval value as ulong) as string
	return chr((value shr 24) and 255, (value shr 16) and 255, (value shr 8) and 255, value and 255)
end function

private function Padded(byref value as const string) as string
	return value + string((4 - (len(value) and 3)) and 3, 0)
end function

private function NameRecord(byval tag as ulong, byref value as const string) as string
	dim aligned as string = Padded(value)
	return LongWord(tag) + LongWord(len(aligned) \ 4) + aligned
end function

private sub Rejected(byref wire as const string)
	dim section as const ubyte ptr = cptr(const ubyte ptr, 1)
	dim count as integer = 123
	if objinfoLoadHunk(cptr(const ubyte ptr, strptr(wire)), len(wire), section, count) then end 1
	if section <> 0 or count <> 0 then end 2
end sub

private sub Accepted(byref wire as const string, byref payload as const string)
	dim section as const ubyte ptr, count as integer
	if not objinfoLoadHunk(cptr(const ubyte ptr, strptr(wire)), len(wire), section, count) then end 3
	if section = 0 or count <> len(payload) then end 4
	for index as integer = 0 to count - 1
		if section[index] <> asc(payload, index + 1) then end 5
	next
end sub

dim payload as string = Padded("-l" + chr(0) + "dependency" + chr(0))
dim metadata as string = NameRecord(1000, ".fbctinf") + LongWord(1002) + _
	LongWord(len(payload) \ 4) + payload + LongWord(1010)
dim wire as string = LongWord(999) + LongWord(0) + metadata
Accepted(wire, payload)
for cut as integer = 0 to len(wire) - 1
	dim prefix as string = left(wire, cut)
	Rejected(prefix)
next

'' These length fields count longwords, not bytes. None may overflow.
for offset as integer = 5 to 29 step 8
	if offset = 21 then continue for
	dim damaged as string = wire
	mid(damaged, offset, 4) = LongWord(&hffffffff)
	Rejected(damaged)
next
dim empty_wire as string
Rejected(empty_wire)
dim false_name as string = LongWord(999) + LongWord(0) + NameRecord(1000, ".fbctinfX") + _
	LongWord(1002) + LongWord(len(payload) \ 4) + payload + LongWord(1010)
Rejected(false_name)

'' Relocation groups, external definitions/references, symbols and debug data
'' can precede the metadata. Their contents must never be scanned as sections.
dim code as string = NameRecord(1000, ".text") + LongWord(1001) + LongWord(1) + LongWord(0)
code += LongWord(1004) + LongWord(1) + LongWord(0) + LongWord(0) + LongWord(0)
code += LongWord(1007) + LongWord(&h01000001) + Padded("def") + LongWord(0)
code += LongWord(&h81000002) + Padded("external") + LongWord(1) + LongWord(0)
code += LongWord(&h82000002) + Padded("common") + LongWord(16) + LongWord(1) + LongWord(0)
code += LongWord(0)
code += LongWord(1008) + LongWord(2) + Padded("local") + LongWord(0) + LongWord(0)
code += LongWord(1009) + LongWord(2) + string(8, 0)
code += LongWord(1020) + string(4, 0)
code += LongWord(1010)
dim combined as string = LongWord(999) + LongWord(0) + code + metadata
Accepted(combined, payload)
for cut as integer = 0 to len(combined) - 1
	dim prefix as string = left(combined, cut)
	Rejected(prefix)
next

'' Bad relocation/symbol counts must fail before reaching the metadata.
for index as integer = 1 to len(code) - 3
	dim damaged as string = LongWord(999) + LongWord(0) + code + metadata
	mid(damaged, 8 + index, 4) = LongWord(&hffffffff)
	dim section as const ubyte ptr, count as integer
	'' Some replacements affect opaque code/debug bytes and remain valid.
	'' Every call must nevertheless stay within the supplied buffer.
	if objinfoLoadHunk(cptr(const ubyte ptr, strptr(damaged)), len(damaged), section, count) then
		Accepted(damaged, payload)
	end if
next

print "NATIVE_HUNK_READER_OK"
end 0

'' end of objinfo-reader.bas
