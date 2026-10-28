/'
	Project: FreeBASIC C backend regression controls
	File: c-auto-descriptor-stores.bas
	Purpose: Check omission of unread automatic fixed-array descriptors.
	Responsibilities: Preserve data initialization, BYDESC access, dynamic
	arrays, object lifetime and repeated procedure/global initialization.
	This file does not remove array storage or qualify another target ABI.
'/
'' TEST_MODE : COMPILE_AND_RUN_OK

#macro CHECK( condition )
	if( (condition) = 0 ) then end 1
#endmacro

dim shared as integer value_calls, constructor_calls, destructor_calls

private function NextValue(byval value as integer) as integer
	value_calls += 1
	return value
end function

private function ThroughDescriptor(values() as integer) as integer
	CHECK(lbound(values) = -2)
	CHECK(ubound(values) = 1)
	values(-2) += 10
	return values(-2) + values(1)
end function

type PlainValue
	low_part as long
	high_part as longint
end type

type LiveValue
	text as string
	declare constructor()
	declare destructor()
end type

constructor LiveValue()
	constructor_calls += 1
	text = "live"
end constructor

destructor LiveValue()
	CHECK(text = "live")
	destructor_calls += 1
end destructor

private function ExerciseArrays() as integer
	#ifdef R34_OPAQUE
	asm
		nop
	end asm
	#endif

	var calls_before = value_calls
	dim values(-2 to 1) as integer = {NextValue(2), NextValue(3), NextValue(5), NextValue(7)}
	CHECK(value_calls = calls_before + 4)
	CHECK(values(-2) + values(-1) + values(0) + values(1) = 17)

	dim matrix(-1 to 0, 2 to 3) as longint
	matrix(-1, 2) = 11
	matrix(0, 3) = 13
	CHECK(matrix(-1, 2) + matrix(0, 3) = 24)

	dim records(0 to 1) as PlainValue
	records(0).low_part = clng(NextValue(17))
	records(1).high_part = NextValue(19)
	CHECK(records(0).low_part + records(1).high_part = 36)
	CHECK(value_calls = calls_before + 6)

	'' BYDESC needs real metadata even though direct element access does not.
	dim checked_values(-2 to 1) as integer = {2, 3, 5, 7}
	CHECK(ThroughDescriptor(checked_values()) = 19)
	CHECK(checked_values(-2) = 12)

	redim as integer dynamic_values(-3 to -1)
	dynamic_values(-3) = 23
	redim preserve dynamic_values(-3 to 1)
	CHECK(dynamic_values(-3) = 23)
	CHECK(lbound(dynamic_values) = -3)
	CHECK(ubound(dynamic_values) = 1)
	erase dynamic_values

	var constructors_before = constructor_calls
	var destructors_before = destructor_calls
	scope
		dim lifetime_values(-1 to 1) as LiveValue
		CHECK(constructor_calls = constructors_before + 3)
		CHECK(lifetime_values(0).text = "live")
	end scope
	CHECK(destructor_calls = destructors_before + 3)

	'' Keep the existing data/descriptor pair when there is no separate data
	'' use. Omitting metadata must not strand previously referenced storage.
	dim unread_values(0 to 1) as integer

	return 42
end function

'' This call is emitted after the procedure's borrowed symbols are released.
dim shared as integer initialized_result

private sub InitializeResult() constructor
	initialized_result = ExerciseArrays()
end sub
CHECK(initialized_result = 42)
CHECK(ExerciseArrays() = 42)

'' end of c-auto-descriptor-stores.bas
