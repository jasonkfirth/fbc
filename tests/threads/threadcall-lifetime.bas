'' Project: FreeBASIC compiler and runtime regression tests
'' File: threadcall-lifetime.bas
'' Purpose: Keep ThreadCall arguments valid after the launching frame exits.
'' Responsibilities: Check captured values, owned strings and borrowed aliases.
'' This file intentionally does NOT contain: libffi or thread implementation code.

' TEST_MODE : COMPILE_AND_RUN_OK

'' Match the hosted targets covered by the existing libffi ThreadCall suite.
#if not (defined(__FB_DOS__) or defined(__FB_WII__) or defined(__FB_ARM__) or _
         defined(__FB_JS__) or defined(__FB_NUTTX__) or defined(__FB_XBOX__))

type Sample
	code as long
	weight as double
end type

dim shared gate as any ptr
dim shared result as long

private function make_text() as string
	return string(96, "q") & chr(0) & "end"
end function

private sub worker cdecl _
	( byref literal_text as string, byref temp_text as string, _
	  byval copied_text as ustring, byval number as long, byval snapshot as Sample, _
	  byref borrowed_text as string, byref borrowed_number as long, borrowed_array() as long )

	mutexlock gate
	if( literal_text <> "literal" ) then result or= 1
	if( temp_text <> make_text() ) then result or= 2
	if( copied_text <> "copied" ) then result or= 4
	if( number <> 173 or snapshot.code <> 629 or snapshot.weight <> 8.25 ) then result or= 8
	if( borrowed_text <> "caller" or borrowed_number <> 71 or borrowed_array(2) <> 42 ) then result or= 16

	literal_text &= " modified"
	temp_text &= " modified"
	copied_text &= " modified"
	borrowed_text &= " modified"
	borrowed_number += 1
	borrowed_array(2) += 2
	mutexunlock gate
end sub

private function launch _
	( byref borrowed_text as string, byref borrowed_number as long, borrowed_array() as long ) as any ptr

	dim text as ustring = "copied"
	dim number as long = 173
	dim snapshot as Sample = (629, 8.25)
	return threadcall worker("literal", make_text(), text, number, snapshot, _
	                         borrowed_text, borrowed_number, borrowed_array())
end function

gate = mutexcreate()
if( gate = 0 ) then end 1
dim borrowed_text as string = "caller"
dim borrowed_number as long = 71
dim borrowed_array(1 to 2) as long = {12, 42}

'' The worker reads nothing until launch(), including its temporary
'' argument slots and local strings, has left the stack.
mutexlock gate
dim child as any ptr = launch(borrowed_text, borrowed_number, borrowed_array())
mutexunlock gate
if( child = 0 ) then end 2
threadwait child
mutexdestroy gate
if( result <> 0 ) then end 10 + result
if( borrowed_text <> "caller modified" or borrowed_number <> 72 or _
    borrowed_array(2) <> 44 ) then end 3

#endif

'' end of threadcall-lifetime.bas
