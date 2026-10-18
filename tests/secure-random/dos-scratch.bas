''
'' FreeBASIC DOS runtime tests: dos-scratch.bas
'' Check that successful secure reads clear their real-mode staging copies.
'' The observer runs before console output or another BASIC I/O operation.
'' This file does not inject entropy or alter emulator program memory.
''
#lang "fb"
#include once "fbrandom.bi"

extern "C"
	declare function TestSecureRandomScratchClear( byval count as uinteger ) as long
end extern

dim as ubyte bytes(0 to 4096)
dim as uinteger sizes(0 to 4) = { 1, 16, 32, 4096, 4097 }
for index as long = 0 to ubound(sizes)
	dim as uinteger observed = sizes(index)
	if observed > 4096 then observed = 4096
	dim as long status = FBC.secureRandom( @bytes(0), sizes(index) )
	dim as long cleared = TestSecureRandomScratchClear( observed )
	if status <> FB.SECURE_RANDOM_OK or cleared = 0 then
		print "DOS_SECURE_RANDOM_SCRATCH_FAIL="; sizes(index)
		end 1
	end if
next
print "DOS_SECURE_RANDOM_SCRATCH=PASS cases=5"
end 0

'' end of dos-scratch.bas
