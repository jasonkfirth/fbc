'' Project: FreeBASIC compiler tests
'' File: semantic-output-test.bas
'' Purpose: Exercise semantic publication under deterministic I/O failures.
'' Responsibilities: Inject failures and check previous output and stream ownership.
'' This file intentionally does NOT contain: semantic records or compiler parsing.

#include once "crt/stdio.bi"
#include once "crt/mem.bi"

enum TEST_FAILURE
	NO_FAILURE
	WRITE_FAILURE
	FLUSH_FAILURE
	CLOSE_FAILURE
	REPLACE_FAILURE
end enum

dim shared as TEST_FAILURE failure
dim shared as integer close_count

declare function testWrite( byval buffer as const any ptr, byval size as uinteger, byval count as uinteger, byval stream as FILE ptr ) as uinteger
declare function testFlush( byval stream as FILE ptr ) as long
declare function testClose( byval stream as FILE ptr ) as long
declare function testReplace( byval source as const zstring ptr, byval destination as const zstring ptr ) as long

'' Replace only the writer's operations. The injected close still consumes
'' the real stream, allowing the fixture to detect a second close.
#define SEMANTIC_OUTPUT_WRITE testWrite
#define SEMANTIC_OUTPUT_FLUSH testFlush
#define SEMANTIC_OUTPUT_CLOSE testClose
#define SEMANTIC_OUTPUT_REPLACE testReplace
#include "tooling/semantic-output.bas"

function testWrite( byval buffer as const any ptr, byval size as uinteger, byval count as uinteger, byval stream as FILE ptr ) as uinteger
	'' A partial write exercises the count check, even without a CRT error bit.
	if( failure = WRITE_FAILURE ) then return fwrite(buffer, size, count \ 2, stream)
	return fwrite(buffer, size, count, stream)
end function

function testFlush( byval stream as FILE ptr ) as long
	dim as long status = fflush(stream)
	if( failure = FLUSH_FAILURE ) then return EOF_
	return status
end function

function testClose( byval stream as FILE ptr ) as long
	close_count += 1
	dim as long status = fclose(stream)
	if( failure = CLOSE_FAILURE ) then return EOF_
	return status
end function

function testReplace( byval source as const zstring ptr, byval destination as const zstring ptr ) as long
	if( failure = REPLACE_FAILURE ) then return -1
	return hReplaceFile(source, destination)
end function

private function checkContents( byref expected as const string ) as integer
	dim as FILE ptr stream = fopen(@"output.tsv", @"rb")
	if( stream = NULL ) then return FALSE
	dim as ubyte buffer(0 to 63)
	dim as uinteger length = fread(@buffer(0), 1, ubound(buffer) + 1, stream)
	if( fclose(stream) <> 0 ) then return FALSE
	if( length <> len(expected) ) then return FALSE
	return memcmp(@buffer(0), strptr(expected), length) = 0
end function

dim as string previous = "previous output" + chr(10), replacement = "replacement output" + chr(10)
for failure = NO_FAILURE to REPLACE_FAILURE
	dim as FILE ptr stream = fopen(@"output.tsv", @"wb")
	if( stream = NULL ) then end 1
	if( fwrite(strptr(previous), 1, len(previous), stream) <> len(previous) ) then end 2
	if( fclose(stream) <> 0 ) then end 3
	dim as any ptr writer = fbSemanticOutputOpen(@"output.tsv")
	if( writer = NULL ) then end 4
	close_count = 0
	dim as long status = fbSemanticOutputWrite(writer, strptr(replacement), len(replacement))
	if( (failure = WRITE_FAILURE) = (status <> 0) ) then end 5
	status = fbSemanticOutputFinish(writer, TRUE)
	if( (failure = NO_FAILURE) <> (status <> 0) ) then end 6
	if( close_count <> 1 ) then end 7
	if( failure = NO_FAILURE ) then
		if( checkContents(replacement) = FALSE ) then end 8
	else
		if( checkContents(previous) = FALSE ) then end 9
	end if
next

'' Abandonment also releases staging without publishing valid-looking output.
failure = NO_FAILURE
dim as any ptr writer = fbSemanticOutputOpen(@"output.tsv")
if( writer = NULL ) then end 10
if( fbSemanticOutputWrite(writer, strptr(replacement), len(replacement)) = 0 ) then end 11
if( fbSemanticOutputFinish(writer, FALSE) = 0 ) then end 12
if( checkContents(previous) = FALSE ) then end 13
if( fbSemanticOutputFinish(NULL, FALSE) = 0 ) then end 14
if( fbSemanticOutputWrite(NULL, NULL, 0) <> 0 ) then end 15

print "semantic publication failures passed"
end 0

'' end of semantic-output-test.bas
