'' Project: FreeBASIC compiler tests
'' File: semantic-source-file-test.bas
'' Purpose: Verify source hashing and ownership through the BASIC observer.
'' Responsibilities: Check digest, byte count, stream position, and changed bytes.
'' This file intentionally does NOT contain: a second hash implementation.

#include once "crt/stdio.bi"
#include "tooling/semantic-source-file.bas"

dim as string filename = command(1), expected = command(2)
dim as ulongint expected_bytes = valulng(command(3))
dim as clong position = valint(command(4))
dim as FILE ptr stream = fopen(strptr(filename), @"rb")
if( stream = NULL ) then end 1
if( fseek(stream, position, SEEK_SET) <> 0 ) then end 2
dim as zstring * 65 digest
dim as ulongint bytes
dim as long status
dim as any ptr revision = fbSemanticSourceOpen(stream, @digest, @bytes, @status)
if( revision = NULL ) then end 3
if( (status <> 1) or (bytes <> expected_bytes) or (digest <> expected) ) then end 4
if( ftell(stream) <> position ) then end 5
if( fbSemanticSourceClose(revision) <> 1 ) then end 6
if( ftell(stream) <> position ) then end 7
revision = fbSemanticSourceOpen(stream, @digest, @bytes, @status)
if( revision = NULL ) then end 8

dim as FILE ptr changed = fopen(strptr(filename), @"ab")
if( changed = NULL ) then end 9
if( fputs(@"changed", changed) < 0 ) then end 10
if( fclose(changed) <> 0 ) then end 11
if( fbSemanticSourceClose(revision) <> 0 ) then end 12
if( ftell(stream) <> position ) then end 13
if( fclose(stream) <> 0 ) then end 14
if( fbSemanticSourceClose(NULL) <> 0 ) then end 15
if( fbSemanticSourceOpen(NULL, @digest, @bytes, @status) <> NULL ) then end 16

print "semantic source revision passed"
end 0

'' end of semantic-source-file-test.bas
