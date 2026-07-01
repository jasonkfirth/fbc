''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbcount.bas
''
'' Purpose:
''
''     Confirm that a second generated-C FreeBASIC program can coexist with
''     fbhello in the same NuttX image and return control to NSH.
''
'' Responsibilities:
''
''     - print a short counter sequence
''     - avoid console input so serial automation can run it safely
''
'' This file intentionally does NOT contain:
''
''     - timing-sensitive checks
''     - target-specific compiler switches
''

dim as integer i

print "fbcount: counting"

for i = 1 to 4
    print "fbcount:"; i
next

print "fbcount: done"

'' end of fbcount.bas
