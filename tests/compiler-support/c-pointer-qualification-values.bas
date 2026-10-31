/'
	Project: FreeBASIC C backend regression controls
	File: c-pointer-qualification-values.bas
	Purpose: Preserve qualified pointer values through frontend-selected writes.
	Responsibilities: Explicit conversions, lazy operands, unaligned stores,
	null pointers, constructor arrays and virtual destructor dispatch.
	This file intentionally does NOT contain: Writes to immutable storage,
	allocation failure injection or changes to public C pointer qualifiers.
'/
#lang "fb"

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_POINTER_QUALIFICATION_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as long address_calls, flag_calls, qualified_reads
dim shared values(0 to 2) as long = { 11, 22, 33 }
dim shared as long static_target = 71
dim shared as long ptr static_view = cptr(long ptr, cptr(const long ptr, @static_target))

declare function PointerSource cdecl alias "fpqual_source"(byval index_value as long) as const long ptr
declare sub InspectQualified cdecl alias "fpqual_inspect"(byval value as const long ptr)
declare sub ModifyValue cdecl alias "fpqual_modify"(byval value as long ptr, byval replacement as long)

function PointerSource cdecl(byval index_value as long) as const long ptr
	VERIFY(index_value >= 0 and index_value <= 2)
	address_calls += 1
	return @values(index_value)
end function

sub InspectQualified cdecl(byval value as const long ptr)
	qualified_reads += *value
end sub

sub ModifyValue cdecl(byval value as long ptr, byval replacement as long)
	*value = replacement
end sub

function RuntimeFlag(byval enabled as long) as long
	flag_calls += 1
	return enabled
end function

type PackedValue field = 1
	prefix as ubyte
	value as long
	suffix as ubyte
end type

dim shared as long plain_ctors, plain_dtors
type PlainValue
	value as long
	declare constructor()
	declare destructor()
end type

constructor PlainValue()
	plain_ctors += 1
	value = 37
end constructor

destructor PlainValue()
	VERIFY(value = 37)
	plain_dtors += 1
end destructor

dim shared as long base_ctors, base_dtors, derived_ctors, derived_dtors
type BaseValue extends object
	value as long
	declare constructor()
	declare virtual destructor()
end type

type DerivedValue extends BaseValue
	declare constructor()
	declare destructor() override
end type

constructor BaseValue()
	base_ctors += 1
	value = 59
end constructor

destructor BaseValue()
	VERIFY(value = 59)
	base_dtors += 1
end destructor

constructor DerivedValue()
	derived_ctors += 1
end constructor

destructor DerivedValue()
	derived_dtors += 1
end destructor

#ifdef CHECK_OPAQUE
	asm
		nop
	end asm
#endif

'' The underlying storage is writable; the read-only view remains qualified.
VERIFY(*static_view = 71)
*static_view = 73
VERIFY(static_target = 73)
dim as const long ptr readonly_value = @values(0)
InspectQualified(readonly_value)
VERIFY(qualified_reads = 11)
*cptr(long ptr, readonly_value) = 41
VERIFY(*readonly_value = 41)
VERIFY(cptr(long ptr, readonly_value) = @values(0))

address_calls = 0
ModifyValue(cptr(long ptr, PointerSource(1)), 43)
VERIFY(address_calls = 1)
VERIFY(values(1) = 43)

'' Each selected operand is evaluated once. Skipped operands must stay skipped
'' even when the C emitter hoists storage for a pointer conversion's result.
address_calls = 0
dim as const long ptr selected_value = iif(RuntimeFlag(1), PointerSource(2), PointerSource(0))
VERIFY(address_calls = 1)
VERIFY(*selected_value = 33)
*cptr(long ptr, selected_value) = 47
VERIFY(values(2) = 47)
VERIFY((RuntimeFlag(0) andalso (*cptr(long ptr, PointerSource(0)) = 41)) = 0)
VERIFY(address_calls = 1)
VERIFY(flag_calls = 2)

dim as const long ptr empty_value = 0
VERIFY(cptr(long ptr, empty_value) = 0)

'' The packed value begins one byte after its record's address. Copying a
'' pointer value must neither align that address nor touch either sentinel.
dim as PackedValue packed = (&hA5, 53, &h5A)
VERIFY(sizeof(PackedValue) = 6)
dim as const long ptr packed_address = cptr(const long ptr, cptr(ubyte ptr, @packed) + 1)
*cptr(long ptr, packed_address) = 61
VERIFY(packed.prefix = &hA5)
VERIFY(packed.suffix = &h5A)
VERIFY(*packed_address = 61)

scope
	dim as const PlainValue local_value = PlainValue()
	VERIFY(local_value.value = 37)
end scope
VERIFY(plain_ctors = 1 and plain_dtors = 1)
dim as const PlainValue ptr array_values = new const PlainValue[3]
VERIFY(array_values <> 0)
for i as long = 0 to 2
	VERIFY(array_values[i].value = 37)
next
delete[] array_values
VERIFY(plain_ctors = 4 and plain_dtors = 4)

dim as const BaseValue ptr base_value = new const BaseValue
VERIFY(base_value <> 0)
delete base_value
VERIFY(base_ctors = 1 and base_dtors = 1)

dim as const BaseValue ptr derived_value = new DerivedValue
VERIFY(derived_value <> 0)
delete derived_value
VERIFY(base_ctors = 2 and base_dtors = 2)
VERIFY(derived_ctors = 1 and derived_dtors = 1)

dim as const BaseValue ptr empty_object = 0
delete empty_object
VERIFY(base_dtors = 2 and derived_dtors = 1)
print "C_POINTER_QUALIFICATION_PASS"

/' end of c-pointer-qualification-values.bas '/
