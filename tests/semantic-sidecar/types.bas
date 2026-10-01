'' Project: FreeBASIC semantic sidecar tests
'' File: types.bas
'' Purpose: Exercise finalized layouts, qualifiers, arrays, and constant values.
'' Responsibilities: Supply independent layout and value expectations.
'' This file intentionally does NOT contain: runtime allocation or test assertions.

type Packed field = 1
	first as ubyte
	second as long
	third as double
end type

union Overlay
	word as ulong
	halfwords(0 to 1) as ushort
end union

type Bits
	first : 3 as uinteger
	second : 5 as uinteger
end type

type BaseType extends Object
	declare virtual function ReadValue() as long
protected:
	protected_value as long
private:
	private_value as long
end type

type DerivedType extends BaseType
	declare function ReadValue() as long override
end type

function BaseType.ReadValue() as long
	return protected_value
end function

function DerivedType.ReadValue() as long
	return 3
end function

type ForwardAlias as LaterType
type LaterType
	value as long
end type

const SignedValue as longint = -9223372036854775807ll
const UnsignedValue as ulongint = &hffffffffffffffffull
const FloatingValue as double = 1.5
const ByteText = !"A\0B\t%\n"
const WideText = wstr(!"A\0B")

dim shared fixed_array(-2 to 2, 3 to 5) as long
redim shared dynamic_array(1 to 4) as double
dim shared unknown_array() as long
dim shared packed_value as Packed
dim shared forward_value as ForwardAlias
dim shared text_value as string
dim shared unicode_value as ustring
dim shared fixed_text as string * 12
dim shared byte_text as zstring * 12
dim shared wide_text as wstring * 12
dim shared pointer_value as const long ptr
dim shared pointer_array(0 to 1) as long ptr
dim shared callback_value as function(byval value as long) as long
extern external_value alias "external%name" as long

'' end of types.bas
