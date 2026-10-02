'' Project: FreeBASIC examples
'' File: ustring-wire.bas
'' Purpose: Store all three string types in a portable UTF-8 wire format.
'' Responsibilities: Demonstrate byte limits, Unicode paths, and bounded reads.
'' This file intentionally does NOT open sockets or choose a network protocol.

#include once "fbnetwire.bi"
#include once "file.bi"

dim as ustring filename = "ustring-" + uchr(&hE9) + ".tmp"
dim as ustring text = uchr(&hE9, &h1F600, 65), received
dim as wstring * 32 wide = wstr(text), wideResult
dim as string bytes = cast(string, text)
dim as integer handle = freefile()

if open(filename for binary as #handle) <> 0 then end 1
if FbNetPutStringLE(handle, bytes, 7) = 0 then end 1
if FbNetPutStringLE(handle, wide, 7) = 0 then end 1
'' The five-byte limit retains é and excludes the incomplete emoji.
if FbNetPutStringLE(handle, text, 5) = 0 then end 1
seek #handle, 1
if FbNetGetStringLE(handle, received, 7) = 0 then end 1
print "Read UTF-8: "; received
'' Wide output needs its buffer capacity, including the terminator.
if FbNetGetStringLE(handle, wideResult, 7, 32) = 0 then end 1
print "Read wide: "; wideResult
if FbNetGetStringLE(handle, received, 7) = 0 then end 1
print "Complete scalars within the byte limit: "; received
close #handle
print "File exists through a wide path: "; FileExists(wstr(filename))
print "Bytes on disk: "; FileLen(filename)
kill filename

'' end of ustring-wire.bas
