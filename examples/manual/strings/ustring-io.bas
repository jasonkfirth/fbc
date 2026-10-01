'' Project: FreeBASIC examples
'' File: ustring-io.bas
'' Purpose: Use UTF-8 text in records, arrays, procedures, and files.
'' Responsibilities: Demonstrate descriptor lifetime and ordinary text I/O.
'' This file intentionally does NOT contain platform console configuration.

type Greeting
	text as ustring
end type

function makeGreeting( byref personName as const ustring ) as ustring
	return "Hello " + personName + " " + uchr(&h1F642)
end function

dim as ustring names(0 to 1) = { "Zo" + uchr(&hEB), uchr(&h4E2D) }
dim as Greeting message
message.text = makeGreeting(names(0))
print message.text
swap names(0), names(1)
print makeGreeting(names(0))

'' Text files contain UTF-8 bytes. LEN still counts code points in memory.
const filename = "ustring-example.txt"
dim as integer handle = freefile()
open filename for output encoding "utf-8" as #handle
print #handle, message.text
write #handle, names(0)
close #handle

dim as ustring readLine, readName
open filename for input encoding "utf-8" as #handle
line input #handle, readLine
input #handle, readName
close #handle
print "Read "; len(readLine); " code points: "; readLine
print "Read quoted name: "; readName
kill filename

'' end of ustring-io.bas
