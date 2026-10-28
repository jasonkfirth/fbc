'' Project: FreeBASIC semantic sidecar tests
'' File: native-blocks.bas
'' Purpose: Exercise native scope owners, surviving exits and cleanup order.
'' Responsibilities: Nested storage, forward transfers, returns and loop exits.
'' This file intentionally does NOT infer cleanup from source variable order.

type BlockObject
	value as integer
	declare constructor( byval number as integer )
	declare destructor( )
end type

constructor BlockObject( byval number as integer )
	value = number
end constructor

destructor BlockObject( )
	print value
end destructor

private sub UnusedBlockBody( )
	scope
		dim unused_block_value as long
		unused_block_value = 7
	end scope
end sub

function NativeBlockExit( byval mode as integer ) as integer
	dim root_block as BlockObject = BlockObject(10)
	dim plain_block as long
	scope
		dim outer_block as BlockObject = BlockObject(20)
		scope
			dim inner_block as BlockObject = BlockObject(30)
			if mode = 1 then return 7
			if mode = 2 then goto PastBlockScope
		end scope
	end scope
PastBlockScope:
	for index as integer = 0 to 2
		if index = 0 then continue for
		exit for
	next
	do
		exit do
	loop
	while mode > 0
		exit while
	wend
	NativeBlockExit = 11
end function

print NativeBlockExit(1)

'' end of native-blocks.bas
