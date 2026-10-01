' TEST_MODE : COMPILE_AND_RUN_OK

'' FreeBASIC Compiler Test Suite
'' File: llvm-null-check-control-flow.bas
'' Purpose: Check SSA control flow around generated null-pointer checks.
'' Responsibilities: Exercise pointer checks inside a conditional while
'' passing a global string address to a subsequent method call.
'' This file does not test a failing null check or error-handler recovery.

#cmdline "-enullptr"

type Node
	value as long
	declare function readValue( byref label as string ) as long
end type

type Root
	child as Node ptr
end type

dim shared item as Node = (42)
dim shared tree as Root = (@item)
dim shared treeptr as Root ptr = @tree
dim shared label as string

function Node.readValue( byref label as string ) as long
	if( label <> "node" ) then end 1
	return value
end function

function lookup( byval enabled as long ) as long
	if( enabled ) then
		return treeptr->child->readValue(label)
	end if
	return -1
end function

label = "node"
if( lookup(0) <> -1 ) then end 2
if( lookup(1) <> 42 ) then end 3
end 0

'' end of llvm-null-check-control-flow.bas
