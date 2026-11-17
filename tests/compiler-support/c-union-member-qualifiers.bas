/'
    Project: FreeBASIC C backend regression
    File: c-union-member-qualifiers.bas
    Purpose: Preserve pointer types when overlapping union members are recovered.
    Responsibilities: Exercise nested, packed and array slots and raw-word copies.
    The native observer owns independent layout and byte-preservation checks.
'/
#lang "fb"

union PointerChoice
	readOnlyLong as const long ptr
	writeLong as long ptr
	readOnlyShort as const short ptr
	writeShort as short ptr
	anyValue as any ptr
end union

type ChoiceRecord
	prefix as uinteger
	items(0 to 1) as PointerChoice
	suffix as uinteger
end type

type PackedChoiceRecord field = 1
	prefix as ubyte
	items(0 to 1) as PointerChoice
	suffix as uinteger
end type

function unionMutableSlot cdecl alias "probe_union_mutable_slot" _
	(byval record as ChoiceRecord ptr, byval index as long) as long ptr ptr
	if index = 0 then return cptr(long ptr ptr, @record->items(0).writeLong)
	return cptr(long ptr ptr, @record->items(1).writeLong)
end function

function unionPackedMutableSlot cdecl alias "probe_union_packed_mutable_slot" _
	(byval record as PackedChoiceRecord ptr, byval index as long) as long ptr ptr
	if index = 0 then return cptr(long ptr ptr, @record->items(0).writeLong)
	return cptr(long ptr ptr, @record->items(1).writeLong)
end function

function unionConstSlot cdecl alias "probe_union_const_slot" _
	(byval record as ChoiceRecord ptr) as const long ptr ptr
	return @record->items(0).readOnlyLong
end function

sub unionStore cdecl alias "probe_union_store" _
	(byval record as ChoiceRecord ptr, byval index as long, byval value as long ptr)
	*unionMutableSlot(record, index) = value
end sub

sub unionPackedStore cdecl alias "probe_union_packed_store" _
	(byval record as PackedChoiceRecord ptr, byval index as long, byval value as long ptr)
	*unionPackedMutableSlot(record, index) = value
end sub

sub unionShortStore cdecl alias "probe_union_short_store" _
	(byval record as ChoiceRecord ptr, byval value as short ptr)
	dim as short ptr ptr slot = cptr(short ptr ptr, @record->items(0).writeShort)
	*slot = value
end sub

sub unionRawStore cdecl alias "probe_union_raw_store" _
	(byval record as ChoiceRecord ptr, byval value as ulongint)
	'' No integer member exists. The representation-copy fallback owns this word.
	*cptr(uinteger ptr, @record->items(0).anyValue) = cuint(value)
end sub

function unionRawRead cdecl alias "probe_union_raw_read" _
	(byval record as ChoiceRecord ptr) as ulongint
	return *cptr(uinteger ptr, @record->items(0).anyValue)
end function

function unionLayout cdecl alias "probe_union_layout" (byval selector as long) as ulongint
	select case selector
	case 0: return sizeof(PointerChoice)
	case 1: return sizeof(ChoiceRecord)
	case 2: return offsetof(ChoiceRecord, items(0))
	case 3: return offsetof(ChoiceRecord, suffix)
	case 4: return sizeof(PackedChoiceRecord)
	case 5: return offsetof(PackedChoiceRecord, items(0))
	case 6: return offsetof(PackedChoiceRecord, suffix)
	end select
	return 0
end function

#ifdef UNION_MEMBER_CONST_DROP
function unionExplicitConstDrop cdecl alias "probe_union_explicit_const_drop" _
	(byval record as ChoiceRecord ptr) as long ptr ptr
	return cptr(long ptr ptr, @record->items(0).readOnlyLong)
end function
#endif

#ifndef UNION_MEMBER_LIBRARY
dim value as long = 417
dim normal as ChoiceRecord
dim packed as PackedChoiceRecord
unionStore(@normal, 0, @value)
unionPackedStore(@packed, 1, @value)
if normal.items(0).writeLong <> @value then end 1
if packed.items(1).writeLong <> @value then end 2
if *unionConstSlot(@normal) <> @value then end 3
print "UNION_MEMBER_QUALIFIERS_PASS"
#endif

/' end of c-union-member-qualifiers.bas '/
