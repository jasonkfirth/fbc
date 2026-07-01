''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbhello.bas
''
'' Purpose:
''
''     Provide the smallest FreeBASIC program used to confirm that NuttX
''     booted far enough to run a generated-C BASIC app from NSH.
''
'' Responsibilities:
''
''     - print a visible startup message
''     - exercise a tiny integer loop
''     - exit without waiting for console input
''
'' This file intentionally does NOT contain:
''
''     - graphics, audio, file I/O, networking, or threading
''     - target-specific compiler switches
''

print "fbhello: FreeBASIC is running on NuttX/RISC-V"

dim as integer i
dim as integer total

for i = 1 to 5
    total += i
next

print "fbhello: sum 1..5 ="; total
print "fbhello: done"

'' end of fbhello.bas
