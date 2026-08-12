''
'' FreeBASIC RISC OS examples
'' --------------------------
''
'' File: hello.bas
''
'' Purpose:
''
''     Exercise basic console output and generated ARM code on RISC OS.
''
'' Responsibilities:
''
''     - reject accidental compilation for another target
''     - perform a small array and loop calculation
''     - emit an unambiguous success or failure marker for emulator tests
''
'' This file intentionally does NOT contain:
''
''     - graphics or sound initialization
''     - filesystem or network tests
''     - emulator-specific control
''

#if not defined(__FB_RISCOS__)
	#error This example must be compiled with -target arm-unknown-riscos
#endif

dim values(0 to 3) as integer = { 1, 2, 3, 4 }
dim total as integer

for i as integer = lbound(values) to ubound(values)
	total += values(i)
next

print "FreeBASIC for RISC OS"

if( total <> 10 ) then
	print "FB_RISCOS_SMOKE_FAILED total="; total
	end 1
end if

print "FB_RISCOS_SMOKE_OK total="; total

'' end of hello.bas
