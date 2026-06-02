' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC command sweep tests
'' -----------------------------
''
'' File: compiler-command-sweep.bas
''
'' Purpose:
''
''     Compile and run one program that touches broad parser, semantic,
''     type-system, control-flow, preprocessor, and code-generation paths.
''
'' Responsibilities:
''
''     - exercise representative compiler syntax in one stable smoke test
''     - verify a few runtime-visible results from generated code
''     - avoid platform APIs so the test remains global
''
'' This file intentionally does NOT contain:
''
''     - runtime-library command coverage
''     - gfxlib coverage
''     - sfxlib coverage
''     - expected compiler-error coverage
''

#define SWEEP_VALUE 17
#macro SWEEP_ADD( a, b )
	( (a) + (b) )
#endmacro

enum SweepEnum
	SWEEP_ENUM_A = 1
	SWEEP_ENUM_B = 2
end enum

type SweepBase extends object
	declare virtual function value() as integer
end type

type SweepDerived extends SweepBase
	field as integer
	declare constructor( byval initial_value as integer )
	declare function value() as integer
	declare static function static_value() as integer
end type

type SweepPair
	first as integer
	second as integer
end type

union SweepUnion
	i as integer
	b(0 to 3) as ubyte
end union

declare function add_byref( byref lhs as integer, byval rhs as integer = 1 ) as integer
declare function sum_array( values() as integer ) as integer
declare sub use_procptr( byval p as function( byval as integer ) as integer, byref result as integer )
declare function square_value( byval value as integer ) as integer
declare operator + ( byref lhs as SweepPair, byref rhs as SweepPair ) as SweepPair

constructor SweepDerived( byval initial_value as integer )
	this.field = initial_value
end constructor

function SweepBase.value() as integer
	function = 0
end function

function SweepDerived.value() as integer
	function = field
end function

function SweepDerived.static_value() as integer
	function = 23
end function

function add_byref( byref lhs as integer, byval rhs as integer ) as integer
	lhs += rhs
	function = lhs
end function

function sum_array( values() as integer ) as integer
	dim as integer total = 0
	for i as integer = lbound( values ) to ubound( values )
		total += values( i )
	next
	function = total
end function

sub use_procptr( byval p as function( byval as integer ) as integer, byref result as integer )
	result = p( 6 )
end sub

function square_value( byval value as integer ) as integer
	function = value * value
end function

operator + ( byref lhs as SweepPair, byref rhs as SweepPair ) as SweepPair
	operator = type<SweepPair>( lhs.first + rhs.first, lhs.second + rhs.second )
end operator

dim shared as integer failures

sub expect_true( byref label as string, byval value as integer )
	if( value = 0 ) then
		print label; ": condition failed"
		failures += 1
	end if
end sub

sub expect_int( byref label as string, byval actual as integer, byval expected as integer )
	if( actual <> expected ) then
		print label; ": expected "; expected; ", got "; actual
		failures += 1
	end if
end sub

namespace SweepNamespace
	type NestedType
		value as integer
	end type

	function nested_value( byval value as integer ) as integer
		function = value + 3
	end function
end namespace

using SweepNamespace

#assert SWEEP_VALUE = 17
#assert SWEEP_ADD( 2, 3 ) = 5

const COMPILE_TIME_CONST = SWEEP_ADD( SWEEP_VALUE, 5 )
expect_int "const/macro", COMPILE_TIME_CONST, 22

#ifdef SWEEP_VALUE
	expect_true "ifdef", true
#else
	expect_true "ifdef", false
#endif

dim as integer scalar = 10
expect_int "byref/default", add_byref( scalar ), 11
expect_int "byref explicit", add_byref( scalar, 4 ), 15

dim as integer dynamic_values( 0 to 3 ) = { 1, 2, 3, 4 }
expect_int "array argument", sum_array( dynamic_values() ), 10

dim as integer static_value = 5
expect_int "static local", static_value, 5

scope
	dim as SweepDerived derived = SweepDerived( 31 )
	dim as SweepBase ptr base_ptr = @derived
	expect_int "virtual dispatch", base_ptr->value(), 31
	expect_int "static member", SweepDerived.static_value(), 23
end scope

dim as SweepPair a = type<SweepPair>( 1, 2 )
dim as SweepPair b = type<SweepPair>( 3, 4 )
dim as SweepPair c = a + b
expect_int "operator first", c.first, 4
expect_int "operator second", c.second, 6

dim as SweepUnion u
u.i = &h01020304
expect_true "union alias", u.b(0) <> 0 orelse u.b(1) <> 0

dim as NestedType nested = type<NestedType>( 9 )
expect_int "namespace type", nested.value, 9
expect_int "namespace function", nested_value( 4 ), 7

dim as function( byval as integer ) as integer proc_ptr = @square_value
dim as integer proc_result
use_procptr proc_ptr, proc_result
expect_int "procptr", proc_result, 36

dim as integer select_result
select case SWEEP_ENUM_B
case SWEEP_ENUM_A
	select_result = 1
case SWEEP_ENUM_B
	select_result = 2
case else
	select_result = 3
end select
expect_int "select enum", select_result, 2

dim as integer loop_total = 0
for i as integer = 1 to 5
	if( i = 2 ) then continue for
	if( i = 5 ) then exit for
	loop_total += i
next
expect_int "for/continue/exit", loop_total, 8

dim as integer while_count = 0
while while_count < 3
	while_count += 1
wend
expect_int "while", while_count, 3

dim as integer do_count = 0
do
	do_count += 1
loop until do_count = 2
expect_int "do loop", do_count, 2

dim as integer cond = iif( loop_total = 8, 100, 200 )
expect_int "iif", cond, 100

if( failures <> 0 ) then
	end 1
end if

end 0

'' end of compiler-command-sweep.bas
