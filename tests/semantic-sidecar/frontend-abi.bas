'' Project: FreeBASIC semantic sidecar tests
'' File: frontend-abi.bas
'' Purpose: Exercise source formals and frontend ABI classifications separately.
'' Responsibilities: Scalar, reference, descriptor, nontrivial and variadic signatures.
'' This file intentionally does NOT promise machine register or stack locations.

type PlainABI
	value as long
end type

type ManagedABI
	text as string
end type

type LargeABI
	values(0 to 7) as double
end type

declare function ScalarABI cdecl(byval value as byte) as long
declare sub ValueABI cdecl(byval plain as PlainABI, byval managed as ManagedABI, byval text as string)
declare sub ReferenceABI cdecl(byref value as const long ptr, values(any, any) as long)
declare sub VariadicABI cdecl(byval count as long, ...)
declare function LargeResultABI cdecl() as LargeABI
declare function ReferenceResultABI(byref value as PlainABI) byref as PlainABI

type MethodABI extends Object
	declare sub ChangeValue(byref value as long)
end type

sub MethodABI.ChangeValue(byref value as long)
	value += 1
end sub

dim shared callback_abi as function cdecl(byval item as long, ...) as long

'' end of frontend-abi.bas
