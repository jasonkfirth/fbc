/'
	Project: FreeBASIC C backend regression controls
	File: c-private-readonly.bas
	Purpose: Check private read-only bodies without changing their pointer ABI.
	Responsibilities: Direct/indirect calls, one set of static locals, character
	references, mutable and escaping arguments, and opaque assembly boundaries.
	This file intentionally does NOT contain: Filesystem or window dependencies.
'/

#macro VERIFY(condition)
	if (condition) = 0 then
		print "C_PRIVATE_READONLY_FAILED", __LINE__
		end 1
	end if
#endmacro

dim shared as wstring ptr saved_text

private function ReadText(byref text as wstring) as string
	static as integer calls
	calls += 1
	return str(text) & str(calls)
end function

private function ReadBytes(byval text as zstring ptr) as string
	return *text
end function

private function OnlyPointer(byref text as wstring) as string
	return str(text)
end function

private sub WriteText(byref text as wstring)
	text = "changed"
end sub

private sub SaveText(byref text as wstring)
	saved_text = @text
end sub

private sub ForwardWrite(byref text as wstring)
	WriteText(text)
end sub

#ifdef CHECK_OPAQUE
private function OpaqueRead(byref text as wstring) as string
	asm
		nop
	end asm
	return str(text)
end function
#endif

dim as wstring * 32 buffer = "abc"
dim reader as function(byref as wstring) as string = @ReadText
VERIFY(ReadText(wstr("abc")) = "abc1")
VERIFY(reader(buffer) = "abc2")
VERIFY(ReadText(wstr("abc")) = "abc3")
VERIFY(ReadBytes("bytes") = "bytes")
dim other_reader as function(byref as wstring) as string = @OnlyPointer
VERIFY(other_reader(buffer) = "abc")
ForwardWrite(buffer)
VERIFY(buffer = "changed")
SaveText(buffer)
VERIFY(saved_text = @buffer)
#ifdef CHECK_OPAQUE
VERIFY(OpaqueRead(buffer) = "changed")
asm
	lea rax, [rip + ReadBytes]
end asm
#endif
print "C_PRIVATE_READONLY_PASS"

'' end of c-private-readonly.bas
