'' Project: FreeBASIC test suite
'' File: lock-range.bas
'' Purpose: Verify byte-range and random-record LOCK/UNLOCK behavior.
'' Responsibilities: Check matching ranges and cross-process exclusion on Unix.
'' This file intentionally does NOT contain: Tests for unsupported platform locking.
' TEST_MODE : COMPILE_AND_RUN_OK

const DATA_FILE = "lock-range.tmp"
const READY_FILE = "lock-range.ready"
const RELEASE_FILE = "lock-range.release"
const ACQUIRED_FILE = "lock-range.acquired"
const DONE_FILE = "lock-range.done"
const RECORD_LENGTH = 4
const ERROR_ILLEGALFUNCTIONCALL = 1
const ERROR_FILEIO = 3
const POLL_INTERVAL_MS = 10
const MAX_POLL_COUNT = 500

sub writeMarker( byref filename as string )
	open filename for output as #2
	close #2
end sub

sub removeFile( byref filename as string )
	if len( dir( filename ) ) > 0 then
		kill filename
	end if
end sub

#if not defined( __FB_JS__ ) and not defined( __FB_WII__ ) and _
	not defined( __FB_XBOX__ ) and not defined( __FB_WINCE__ ) and _
	not defined( __FB_NUTTX__ )

dim as string mode = command( 1 )

if mode = "--child" or mode = "--record-child" then
	if mode = "--record-child" then
		open DATA_FILE for random access read write as #1 len = RECORD_LENGTH
	else
		open DATA_FILE for binary access read write as #1
	end if

	'' Position 1 is outside the parent's range and must remain available.
	lock #1, 1
	if err <> 0 then end 4
	unlock #1, 1
	if err <> 0 then end 5

	'' A conflicting lock fails promptly instead of waiting while holding FB_LOCK.
	lock #1, 3
		if err <> ERROR_FILEIO then end 6
	writeMarker( READY_FILE )

	dim as integer child_poll_count = 0
	do while len( dir( RELEASE_FILE ) ) = 0
		sleep POLL_INTERVAL_MS, 1
		child_poll_count += 1
		if child_poll_count >= MAX_POLL_COUNT then end 7
	loop

	'' The parent's UNLOCK must release the exact byte range that it locked.
	lock #1, 3
	if err <> 0 then end 8
	writeMarker( ACQUIRED_FILE )
	unlock #1, 3
	if err <> 0 then end 9
	close #1
	writeMarker( DONE_FILE )
	end 0
end if

removeFile( DATA_FILE )
removeFile( READY_FILE )
removeFile( RELEASE_FILE )
removeFile( ACQUIRED_FILE )
removeFile( DONE_FILE )

open DATA_FILE for binary access read write as #1
put #1, 1, "0123456789"

'' Reject invalid positions and reversed inclusive ranges.
lock #1, 0
if err <> ERROR_ILLEGALFUNCTIONCALL then end 10
unlock #1, 0
if err <> ERROR_ILLEGALFUNCTIONCALL then end 11
lock #1, 3 to 2
if err <> ERROR_ILLEGALFUNCTIONCALL then end 12
unlock #1, 3 to 2
if err <> ERROR_ILLEGALFUNCTIONCALL then end 13

'' The short form locks one byte, while an explicit range includes both ends.
lock #1, 1
if err <> 0 then end 14
unlock #1, 1
if err <> 0 then end 15
lock #1, 1 to 1
if err <> 0 then end 16
unlock #1, 1 to 1
if err <> 0 then end 17
lock #1, 2 to 4
if err <> 0 then end 18

#if defined( __FB_UNIX__ ) or defined( __FB_CYGWIN__ )
dim as string child_command
dim as integer poll_count

child_command = chr( 34 ) & command( 0 ) & chr( 34 ) & " --child"
shell child_command & " >/dev/null 2>&1 &"

poll_count = 0
do while len( dir( READY_FILE ) ) = 0
		sleep POLL_INTERVAL_MS, 1
	poll_count += 1
		if poll_count >= MAX_POLL_COUNT then
		close #1
		end 1
	end if
loop

'' The child reports READY only after the overlapping LOCK returned a conflict.
if len( dir( ACQUIRED_FILE ) ) > 0 then
	close #1
	end 2
end if

unlock #1, 2 to 4
if err <> 0 then end 19
writeMarker( RELEASE_FILE )

poll_count = 0
do while len( dir( ACQUIRED_FILE ) ) = 0
	sleep POLL_INTERVAL_MS, 1
	poll_count += 1
	if poll_count >= MAX_POLL_COUNT then
		'' Closing releases the OS lock so the child test process can exit.
		close #1
		end 3
	end if
loop

poll_count = 0
do while len( dir( DONE_FILE ) ) = 0
	sleep POLL_INTERVAL_MS, 1
	poll_count += 1
	if poll_count >= MAX_POLL_COUNT then end 3
loop
#else
unlock #1, 2 to 4
if err <> 0 then end 19
#endif

close #1

'' Read-only handles need shared locks on Unix, where O_RDONLY is zero.
open DATA_FILE for binary access read as #1
lock #1, 1
if err <> 0 then end 20
unlock #1, 1
if err <> 0 then end 21
close #1

'' Random-file positions are records, and ranges cover each complete record.
removeFile( READY_FILE )
removeFile( RELEASE_FILE )
removeFile( ACQUIRED_FILE )
removeFile( DONE_FILE )
open DATA_FILE for random access read write as #1 len = RECORD_LENGTH
lock #1, 2
if err <> 0 then end 22
unlock #1, 2
if err <> 0 then end 23
lock #1, 2 to 3
if err <> 0 then end 24

#if defined( __FB_UNIX__ ) or defined( __FB_CYGWIN__ )
child_command = chr( 34 ) & command( 0 ) & chr( 34 ) & " --record-child"
shell child_command & " >/dev/null 2>&1 &"

poll_count = 0
do while len( dir( READY_FILE ) ) = 0
		sleep POLL_INTERVAL_MS, 1
	poll_count += 1
		if poll_count >= MAX_POLL_COUNT then
		close #1
		end 24
	end if
loop

if len( dir( ACQUIRED_FILE ) ) > 0 then
	close #1
	end 25
end if

unlock #1, 2 to 3
if err <> 0 then end 26
writeMarker( RELEASE_FILE )

poll_count = 0
do while len( dir( ACQUIRED_FILE ) ) = 0
	sleep POLL_INTERVAL_MS, 1
	poll_count += 1
	if poll_count >= MAX_POLL_COUNT then
		close #1
		end 27
	end if
loop

poll_count = 0
do while len( dir( DONE_FILE ) ) = 0
	sleep POLL_INTERVAL_MS, 1
	poll_count += 1
	if poll_count >= MAX_POLL_COUNT then end 28
loop
#else
unlock #1, 2 to 3
if err <> 0 then end 25
#endif

close #1

#if (defined( __FB_UNIX__ ) or defined( __FB_WIN32__ )) and not defined( __FB_CYGWIN__ )
'' Large positions must retain all high-order offset bits.
open DATA_FILE for binary access read write as #1
lock #1, 4294967297ll to 4294967299ll
if err <> 0 then end 26
unlock #1, 4294967297ll to 4294967299ll
if err <> 0 then end 27
close #1
#endif

'' Closed and invalid file numbers must report errors instead of dereferencing handles.
lock #1, 1
if err <> ERROR_ILLEGALFUNCTIONCALL then end 28
unlock #1, 1
if err <> ERROR_ILLEGALFUNCTIONCALL then end 29
lock #32767, 1
if err <> ERROR_ILLEGALFUNCTIONCALL then end 30
unlock #32767, 1
if err <> ERROR_ILLEGALFUNCTIONCALL then end 31

removeFile( DATA_FILE )
removeFile( READY_FILE )
removeFile( RELEASE_FILE )
removeFile( ACQUIRED_FILE )
removeFile( DONE_FILE )

#else
	'' These targets cannot enforce byte-range locks and report file I/O errors.
	end 0
#endif

'' end of lock-range.bas
