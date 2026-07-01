''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbgosub_smoke.bas
''
'' Purpose:
''
''     Prove that the NuttX smoke image can use the normal FreeBASIC
''     GOSUB/RETURN runtime implementation.
''
'' Responsibilities:
''
''     - run a small fblite GOSUB/RETURN program under NuttX
''     - print a stable marker for the QEMU smoke harness
''
'' This file intentionally does NOT contain:
''
''     - graphics, audio, storage, or networking checks
''     - broader rtlib command coverage
''

#lang "fblite"

option gosub

dim shared as integer gosub_total

gosub first_probe
gosub second_probe

if gosub_total = 42 then
    print "FB_NUTTX_GOSUB_SMOKE_OK"
else
    print "FB_NUTTX_GOSUB_SMOKE_BAD"; gosub_total
    end 1
end if

end

first_probe:
gosub_total = 17
return

second_probe:
gosub_total = gosub_total + 25
return

'' end of fbgosub_smoke.bas
