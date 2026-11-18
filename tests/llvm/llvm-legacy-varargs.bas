'' Project: FreeBASIC compiler regression tests
'' File: llvm-legacy-varargs.bas
'' Purpose: Verify the LLVM rejection and supported replacement for raw varargs.
'' Responsibilities: Isolate each legacy builtin and execute the CVA path.
'' This file intentionally does NOT contain: Stack or register layout assumptions.

private function get_value cdecl(byval anchor as long, ...) as long
	#if defined(TEST_VA_FIRST)
		return cint(va_first())
	#elseif defined(TEST_VA_ARG)
		return va_arg(cptr(any ptr, 0), long)
	#elseif defined(TEST_VA_NEXT)
		return cint(va_next(cptr(any ptr, 0), long))
	#else
		dim args as cva_list
		cva_start(args, anchor)
		var result = cva_arg(args, long)
		cva_end(args)
		return result
	#endif
end function

if( get_value(0, 4321L) <> 4321 ) then end 1

'' end of llvm-legacy-varargs.bas
