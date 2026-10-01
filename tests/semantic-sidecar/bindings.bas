'' Project: FreeBASIC semantic sidecar tests
'' File: bindings.bas
'' Purpose: Exercise declarations, references, shadowing, and qualified lookup.
'' Responsibilities: Provide compiler-selected identities for the Python suite.
'' This file intentionally does NOT contain: test runner or exporter code.

namespace Library
	const Answer = 42
	type Item
		value as long
	end type
	enum Shade explicit
		Dark = -3
		Light = 7
	end enum
	type ItemAlias as Item
	dim shared counter as long
	declare function ReadItem(byref item as Item) as long
end namespace

function Library.ReadItem(byref item as Library.Item) as long
	return item.value + Library.Answer
end function

sub ProbeBindings()
	using Library
	dim outer as long = Answer
	dim item as ItemAlias
	item.value = outer
	with item
		.value += 1
	end with
	scope
		dim outer as double = 2.5
		print outer
	end scope
	print outer, Library.ReadItem(item), Shade.Light
	for iterator as long = 0 to 2
		counter += iterator
	next iterator
	goto finished
finished:
	asm
		mov eax, outer
		jmp asm_local
		asm_local:
	end asm
end sub

ProbeBindings()

'' end of bindings.bas
