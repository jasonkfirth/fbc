'' TEST_MODE : COMPILE_AND_RUN_OK
/'
	Project: FreeBASIC C backend regression controls
	File: c-abi-parameter-intent.bas
	Purpose: Exercise ignored ABI slots without changing argument evaluation.
	Responsibilities: Public and callback contracts, direct calls, and object lifetime.
	This file does not require warning suppression or an installed candidate compiler.
'/

#macro CHECK( condition )
	if( (condition) = 0 ) then
		end 1
	end if
#endmacro

dim shared as integer argument_calls, constructor_calls, copy_calls, destructor_calls

type Payload
	value as integer
	text as string
	declare constructor()
	declare constructor(byval initial_value as integer)
	declare constructor(byref source as const Payload)
	declare destructor()
end type

constructor Payload()
	constructor_calls += 1
	value = 0
	text = string(64, "x")
end constructor

constructor Payload(byval initial_value as integer)
	constructor_calls += 1
	value = initial_value
	text = string(64, "x")
end constructor

constructor Payload(byref source as const Payload)
	copy_calls += 1
	value = source.value
	text = source.text
end constructor

destructor Payload()
	CHECK(len(text) = 64)
	destructor_calls += 1
end destructor

function NextArgument(byval value as integer) as integer
	argument_calls += 1
	return value
end function

function PublicIgnored(byval ignored_value as integer, byval result as integer) as integer
	return result
end function

private function CallbackIgnored(byval ignored_value as integer, byval result as integer) as integer
	return result
end function

private function DirectIgnored(byval ignored_value as integer, byval result as integer) as integer
	return result
end function

function UsedParameter(byval value as integer) as integer
	return value + 1
end function

function MakePayload() as Payload
	argument_calls += 1
	return Payload(9)
end function

function ObjectIgnored(byval value as Payload, byval result as integer) as integer
	'' An unused BASIC value still owns its destructor and string cleanup.
	return result
end function

type Callback as function(byval as integer, byval as integer) as integer
'' This initializer also exercises a procedure address observed after definition emission.
dim shared as Callback callback_slot = @CallbackIgnored

CHECK(PublicIgnored(NextArgument(10), NextArgument(11)) = 11)
CHECK(callback_slot(NextArgument(20), NextArgument(21)) = 21)
CHECK(DirectIgnored(NextArgument(30), NextArgument(31)) = 31)
CHECK(argument_calls = 6)
CHECK(UsedParameter(40) = 41)
dim as integer previous_destructors = destructor_calls
CHECK(ObjectIgnored(MakePayload(), NextArgument(51)) = 51)
CHECK(argument_calls = 8)
CHECK(destructor_calls > previous_destructors)

' end of c-abi-parameter-intent.bas
