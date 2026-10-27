'' Project: FreeBASIC compiler tests
'' File: c-label-entries.bas
'' Purpose: Keep BASIC labels referenced only by inline ASM in emitted C.
'' Responsibilities: Exercise a live assembler target beside an unused label.
'' This file intentionally does NOT contain platform I/O or raw assembler labels.
'' TEST_MODE : COMPILE_AND_RUN_OK
#lang "fb"

private function asm_label_entry( ) as long

	goto dispatch

unused_target:
	return 199

asm_target:
	return 42

dispatch:
	#if defined(__FB_X86__)
		asm
			jmp asm_target
		end asm
	#else
		goto asm_target
	#endif
	return 99

end function

if( asm_label_entry( ) <> 42 ) then end 1
print "c-label-entries-ok"
end 0

'' end of c-label-entries.bas
