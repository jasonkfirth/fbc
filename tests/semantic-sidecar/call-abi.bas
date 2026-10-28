'' Project: FreeBASIC semantic sidecar tests
'' File: call-abi.bas
'' Purpose: Exercise selected argument values and backend ABI call components.
'' Responsibilities: Splits, aggregates, modes, variadic values and dispatch.
'' This file intentionally does NOT predict physical registers or stack slots.

type CallLarge
	values(0 to 7) as double
end type

dim shared call_sum as long

function CallWide cdecl(byval value as integer<128>) as integer<128>
	return value + 1
end function

sub CallMemory cdecl(byval value as CallLarge)
	call_sum += clng(value.values(0))
end sub

sub CallVaList cdecl(byval value as cva_list)
end sub

sub CallVariadic cdecl(byval count as long, ...)
end sub

function CallLeaf(byval value as long) as long
	return value + 1
end function

sub CallModes(byref value as long, byval wide_value as wstring ptr, items() as long)
	value += 1
	items(0) = asc(wide_value[0])
end sub

sub CallOptional(byval value as long = 17)
	call_sum += value
end sub

sub CallPascal pascal(byval first as byte, byval second as long)
	call_sum += first * 10 + second
end sub

type CallObject extends object
	value as long
	declare virtual sub Dispatch(byval value as long)
end type

sub CallObject.Dispatch(byval value as long)
	this.value += value
end sub

declare sub CallFree cdecl alias "free"(byval value as const any ptr)
declare sub CallUnused(byval value as double)

dim callback as function cdecl(byval value as integer<128>) as integer<128> = @CallWide
dim result as integer<128> = CallWide(11)
result = callback(result)
dim aggregate_value as CallLarge
aggregate_value.values(0) = 5
CallMemory(aggregate_value)
dim argument_list as cva_list
CallVaList(argument_list)
CallVariadic(3, cbyte(1), csng(1.25), cshort(3))
dim value as long
dim wide_value as wstring * 4 = "A"
dim items(0 to 2) as long
CallModes(value, @wide_value, items())
CallOptional()
CallOptional(17)
CallPascal(1, 2)
dim instance as CallObject
dim instance_pointer as CallObject ptr = @instance
instance_pointer->Dispatch(CallLeaf(2))
CallFree(0)
if result <> 13 or value <> 1 or items(0) <> 65 or instance.value <> 3 or call_sum <> 51 then end 1
print "CALL_ABI_PASS"

'' end of call-abi.bas
