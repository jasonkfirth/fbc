' TEST_MODE : COMPILE_AND_RUN_OK

'' FreeBASIC Compiler Test Suite
'' File: llvm-static-aggregates.bas
'' Purpose: Check static aggregate types, union storage, and procedure addresses.
'' Responsibilities: Exercise nested TYPE initializers, array scope flags,
'' union padding/alignment, and initialized function pointers at run time.
'' This file does not test dynamic initialization or constructors.

type Location field = 1
	line as long
end type

function plusOne( byval value as long ) as long
	return value + 1
end function

type Node
	index as long
	position as Location
	callback as function( byval as long ) as long
end type

#define nodeInit(n) type<Node>( n, type<Location>( n + 100 ), @plusOne )

'' More than 32 elements also checks that merged initializer offsets cannot
'' change an array/struct scope flag as the element size accumulates.
dim shared nodes(0 to 39) as Node = { _
	nodeInit(0), nodeInit(1), nodeInit(2), nodeInit(3), nodeInit(4), _
	nodeInit(5), nodeInit(6), nodeInit(7), nodeInit(8), nodeInit(9), _
	nodeInit(10), nodeInit(11), nodeInit(12), nodeInit(13), nodeInit(14), _
	nodeInit(15), nodeInit(16), nodeInit(17), nodeInit(18), nodeInit(19), _
	nodeInit(20), nodeInit(21), nodeInit(22), nodeInit(23), nodeInit(24), _
	nodeInit(25), nodeInit(26), nodeInit(27), nodeInit(28), nodeInit(29), _
	nodeInit(30), nodeInit(31), nodeInit(32), nodeInit(33), nodeInit(34), _
	nodeInit(35), nodeInit(36), nodeInit(37), nodeInit(38), nodeInit(39) }

union Pair
	type
		x as long
		y as long
	end type
	values(0 to 1) as long
end union

dim shared pairs(0 to 1) as Pair = { type<Pair>(11, 22), type<Pair>(33, 44) }

union Storage
	first as ubyte
	larger(0 to 1) as double
end union

type Container
	prefix as ubyte
	data as Storage
	suffix as long
end type

dim shared containers(0 to 1) as Container = { _
	type<Container>(1, type<Storage>(2), 3), _
	type<Container>(4, type<Storage>(5), 6) }

for i as long = 0 to 39
	if( nodes(i).index <> i ) then end 1
	if( nodes(i).position.line <> i + 100 ) then end 2
	if( nodes(i).callback(i) <> i + 1 ) then end 3
next

if( pairs(0).values(0) <> 11 or pairs(0).values(1) <> 22 ) then end 4
if( pairs(1).values(0) <> 33 or pairs(1).values(1) <> 44 ) then end 5
pairs(1).values(0) = 55
if( pairs(1).x <> 55 ) then end 6

for i as long = 0 to 1
	if( containers(i).prefix <> 1 + i * 3 ) then end 7
	if( containers(i).data.first <> 2 + i * 3 ) then end 8
	if( containers(i).suffix <> 3 + i * 3 ) then end 9
	'' Bytes outside the initialized union member must be zeroed.
	for j as long = 1 to sizeof(Storage) - 1
		if( cptr(ubyte ptr, @containers(i).data)[j] <> 0 ) then end 10
	next
next

end 0

'' end of llvm-static-aggregates.bas
