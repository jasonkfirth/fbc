'' Project: FreeBASIC runtime tests
'' File: unix-console-runtime.bas
'' Purpose: Exercise console output, view clearing and counted line input.
'' Responsibilities: Run the ordinary BASIC entry points against a real PTY.
'' Private geometry and allocation controls belong to unix-console-bounds.c.
#lang "fb"

dim narrowText as string
dim wideText as wstring * 32
dim wideBlock as wstring * 4097

width 40, 12
color 7, 0
view print 2 to 4
cls 2
view print
locate 1, 1
print "CONSOLE_BEGIN"
print wchr(&h41, &he9, &h20ac)
for i as integer = 0 to 4095
	wideBlock[i] = 65
next
wideBlock[4096] = 0
print wideBlock;
line input ; "NARROW>", narrowText
if narrowText <> "narrow-line" then end 10
line input ; "WIDE>", wideText
if wideText <> wstr("wide-line") then end 11
print "CONSOLE_RUNTIME_PASS"

'' end of unix-console-runtime.bas
