/'
	Project: FreeBASIC C backend regression controls
	File: c-generated-leaf-dependencies.bas
	Purpose: Exercise generated leaves beside conversions and array adapters.
	Responsibilities: Check object copies, callback roots and array lifetimes.
	This file does not qualify runtime error unwinding or external libraries.
'/
'' TEST_MODE : COMPILE_AND_RUN_OK

#macro CHECK( condition )
	if( (condition) = 0 ) then end 1
#endmacro

type DormantPayload
	text as string
	value as integer
end type

type CopyPayload
	text as string
	value as integer
end type

dim shared as integer constructors_seen, destructors_seen, argument_calls

type PublicElement
	text as string
	declare constructor()
	declare destructor()
end type

constructor PublicElement()
	constructors_seen += 1
	text = "live"
end constructor

destructor PublicElement()
	CHECK(text = "live")
	destructors_seen += 1
end destructor

type InternalElement
	text as string
end type

private function CallbackValue(byval value as integer) as integer
	argument_calls += 1
	return value + 1
end function

type ValueCallback as function(byval as integer) as integer
'' This address is emitted with global initializers after procedure bodies.
dim shared as ValueCallback callback_slot = @CallbackValue

function ConversionInput() as double
	return 2.75
end function

private function CopyValue(byval source as CopyPayload) as CopyPayload
	dim as CopyPayload result = source
	result.text += ":copy"
	return result
end function

dim as CopyPayload first, second
first.text = "original"
first.value = 41
second = CopyValue(first)
CHECK(second.text = "original:copy")
CHECK(second.value = 41)
CHECK(first.text = "original")
CHECK(callback_slot(second.value) = 42)
CHECK(argument_calls = 1)

dim as long rounded_value = clng(ConversionInput())
CHECK(rounded_value = 3)

redim as PublicElement public_elements(0 to 2)
CHECK(constructors_seen = 3)
CHECK(destructors_seen = 0)
erase public_elements
CHECK(destructors_seen = 3)

'' String fields require private generated initialization/destruction methods.
redim as InternalElement internal_elements(0 to 1)
internal_elements(0).text = "retained"
redim preserve internal_elements(0 to 3)
CHECK(internal_elements(0).text = "retained")
erase internal_elements

#ifdef R33_OPAQUE
asm
	nop
end asm
#endif

'' end of c-generated-leaf-dependencies.bas
