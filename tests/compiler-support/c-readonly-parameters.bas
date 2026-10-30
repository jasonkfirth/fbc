/'
	Project: FreeBASIC C backend regression controls
	File: c-readonly-parameters.bas
	Purpose: Exercise inferred pointer contracts without changing public types.
	Responsibilities: Check direct and indirect calls, shared static storage,
	and pointer writes, aliases, rebinding and allocation lifetime boundaries.
	This file intentionally does NOT contain: Window or filesystem dependencies.
'/

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_READONLY_CONTROL_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as zstring ptr saved_text

function ReadText(byval text as zstring ptr) as string
	static as integer calls
	calls += 1
	return *text + str(calls)
end function

sub WriteText(byval text as zstring ptr)
	*text = "changed"
end sub

sub SaveText(byval text as zstring ptr)
	saved_text = text
end sub

function ReturnText(byval text as zstring ptr) as zstring ptr
	return text
end function

sub RebindText(byref text as zstring ptr)
	text = saved_text
end sub

function ForwardBinding(byval text as zstring ptr) as string
	RebindText(text)
	return *text
end function

sub DisposeText(byval text as zstring ptr)
	deallocate(text)
end sub

#ifndef R73_HELPER_ONLY
dim as zstring * 32 buffer = "abc"
dim reader as function(byval as zstring ptr) as string = @ReadText
VERIFY(ReadText("abc") = "abc1")
VERIFY(reader(@buffer) = "abc2")
VERIFY(ReadText("abc") = "abc3")
WriteText(@buffer)
VERIFY(buffer = "changed")
SaveText(@buffer)
VERIFY(ReturnText(@buffer) = @buffer)
dim as zstring * 32 other_text = "other"
VERIFY(ForwardBinding(@other_text) = "changed")
DisposeText(cptr(zstring ptr, callocate(32)))
print "C_READONLY_PARAMETERS_PASS"
#endif

'' end of c-readonly-parameters.bas
