'' Project: FreeBASIC compiler regression tests
'' File: llvm-source-debug.bas
'' Purpose: Exercise LLVM debug information through native assembly and linking.
'' Responsibilities: Provide a procedure, parameter, block, aggregate and include.
'' This file intentionally does NOT contain: Debugger command-line options.

type Payload
	items(1 to 2) as long
	link as Payload ptr
end type

sub debug_probe(byval argument as long)
	dim local_value as long = argument + 20
	scope
		dim inner_value as long = local_value + 1
		dim state as Payload
		state.items(1) = inner_value
		select case as const state.items(1)
		case 43
			#include "llvm-source-debug.bi"
		end select
	end scope
end sub

debug_probe(22)

'' end of llvm-source-debug.bas
