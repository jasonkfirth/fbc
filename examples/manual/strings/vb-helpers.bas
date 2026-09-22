'/ FreeBASIC Runtime Library
'/ File: examples/manual/strings/vb-helpers.bas
'/ Purpose: Demonstrate the optional Visual Basic-compatible string helpers.
'/ Responsibilities: Exercise text comparison, replacement, and reversal.
'/ This file does not depend on a graphics driver or external library.

#include once "string.bi"

dim as string sample
dim as string find_text
dim as string replacement

sample = "Aba"
find_text = "a"
replacement = "x"

if StrComp( "Hello", "hello", fbTextCompare ) <> 0 then end 1
if Replace( sample, find_text, replacement, 1, -1, fbTextCompare ) <> "xbx" then end 2
if StrReverse( "Optical" ) <> "lacitpO" then end 3

print "VB string helpers passed"

'/ end of examples/manual/strings/vb-helpers.bas
