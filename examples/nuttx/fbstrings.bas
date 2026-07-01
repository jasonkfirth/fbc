''
'' Project: FreeBASIC NuttX examples
'' ---------------------------------
''
'' File: fbstrings.bas
''
'' Purpose:
''
''     Exercise a small string path on NuttX without using files, input, or
''     other services that can obscure first-boot failures.
''
'' Responsibilities:
''
''     - concatenate short strings
''     - report length and substring results
''     - exit without waiting for console input
''
'' This file intentionally does NOT contain:
''
''     - heap stress testing
''     - filesystem or console input coverage
''

dim as string target_name = "NuttX"
dim as string message_text = "FreeBASIC on " + target_name

print "fbstrings: "; message_text
print "fbstrings: len ="; len(message_text)
print "fbstrings: mid = "; mid(message_text, 14, 5)
print "fbstrings: done"

'' end of fbstrings.bas
