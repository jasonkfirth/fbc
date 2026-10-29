'' Project: FreeBASIC C emitter regression tests
'' File: local-abi-assertions.bas
'' Purpose: Require independent ABI checks for local UDT declarations.
'' Responsibilities: Exercise two local layouts and their generated field accesses.
'' This file intentionally does NOT depend on a particular host pointer width.
' TEST_MODE : COMPILE_AND_RUN_OK

private function check_layouts() as longint
	type FirstLocal
		tag as ubyte
		value as longint
	end type
	type SecondLocal
		tag as byte
		value as integer
	end type

	dim as FirstLocal first
	dim as SecondLocal second
	first.tag = 3
	first.value = 39
	second.tag = 1
	second.value = -1
	return first.tag + first.value + second.tag + second.value
end function

if check_layouts() <> 42 then end 1

'' end of local-abi-assertions.bas
