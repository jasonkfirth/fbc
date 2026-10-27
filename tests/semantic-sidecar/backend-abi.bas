'' Project: FreeBASIC semantic sidecar tests
'' File: backend-abi.bas
'' Purpose: Exercise actual C/LLVM signature adaptations and component order.
'' Responsibilities: Split scalars, memory arguments, va_list, callbacks and aliases.
'' This file intentionally does NOT predict machine register allocation.

type BackendLarge
	values(0 to 7) as double
end type

function WideBackend cdecl(byval value as integer<128>) as integer<128>
	return value
end function

sub MemoryBackend cdecl(byval value as BackendLarge)
	dim copy_value as double = value.values(0)
end sub

sub VaListBackend cdecl(byval value as cva_list)
end sub

sub VariadicBackend cdecl(byval count as long, ...)
end sub

sub PascalBackend pascal(byval first as byte, byval second as long)
end sub

sub ExitBackend cdecl()
end sub

declare function RegisterBackend cdecl alias "atexit"(byval callback as sub cdecl()) as long
declare sub FreeBackend cdecl alias "free"(byval value as const any ptr)
declare sub UnusedBackend(byval value as double)

dim shared callback_backend as function cdecl(byval value as integer<128>) as integer<128> = @WideBackend
RegisterBackend(@ExitBackend)
FreeBackend(0)

'' end of backend-abi.bas
