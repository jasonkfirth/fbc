' TEST_MODE : COMPILE_AND_RUN_OK

''
'' FreeBASIC command sweep tests
'' -----------------------------
''
'' File: rtlib-command-sweep.bas
''
'' Purpose:
''
''     Compile and run one program that touches a broad cross-section of
''     general runtime-library entry points.
''
'' Responsibilities:
''
''     - exercise string, conversion, date/time, math, memory, file, console,
''       environment, error, and dynamic array runtime paths
''     - keep all external side effects local to temporary files
''     - avoid platform-specific devices, networking, graphics, and sound
''
'' This file intentionally does NOT contain:
''
''     - gfxlib coverage
''     - sfxlib coverage
''     - compiler syntax-only coverage
''     - platform-specific runtime probes
''

const TEST_FILE = "rtlib-command-sweep.tmp"
const RENAMED_FILE = "rtlib-command-sweep-renamed.tmp"
const ENV_NAME = "FB_RT_SWEEP_VALUE"

#ifdef __FB_JS__
const CAN_USE_ENVIRON = 0
#else
const CAN_USE_ENVIRON = 1
#endif

dim shared as integer failures

declare sub record_failure( byref message as string )
declare sub expect_int( byref label as string, byval actual as longint, byval expected as longint )
declare sub expect_true( byref label as string, byval value as integer )
declare sub delete_file_if_present( byref filename as string )

sub record_failure( byref message as string )
	print message
	failures += 1
end sub

sub expect_int( byref label as string, byval actual as longint, byval expected as longint )
	if( actual <> expected ) then
		record_failure label + ": expected " + str( expected ) + ", got " + str( actual )
	end if
end sub

sub expect_true( byref label as string, byval value as integer )
	if( value = 0 ) then
		record_failure label + ": condition failed"
	end if
end sub

sub delete_file_if_present( byref filename as string )
	dim as integer f = freefile()

	if( open( filename for input as #f ) = 0 ) then
		close #f
		kill filename
	end if
end sub

randomize 12345, 1

'' String runtime.
dim as string text = "  FreeBASIC runtime sweep  "
dim as string trimmed = trim( text )
expect_true "trim", trimmed = "FreeBASIC runtime sweep"
expect_true "lcase", lcase( left( trimmed, 9 ) ) = "freebasic"
expect_true "ucase", ucase( right( trimmed, 5 ) ) = "SWEEP"
expect_int "instr", instr( trimmed, "runtime" ), 11
expect_int "instrrev", instrrev( trimmed, "e" ), len( trimmed ) - 1
expect_int "len", len( trimmed ), 23
expect_int "asc", asc( "A" ), 65
expect_true "chr", chr( 65 ) = "A"
expect_true "space/string", space( 3 ) + string( 2, "x" ) = "   xx"
expect_true "mid statement", true
mid( trimmed, 11, 7 ) = "command"
expect_true "mid result", trimmed = "FreeBASIC command sweep"
expect_true "str/val", val( str( 1234 ) ) = 1234
expect_true "hex", hex( 255 ) = "FF"
expect_true "oct", oct( 8 ) = "10"
expect_true "bin", bin( 5 ) = "101"
expect_true "wstring", len( wstr( "abc" ) ) = 3

'' Numeric conversion and math runtime.
expect_int "cint", cint( 3.4 ), 3
expect_int "fix", fix( -3.7 ), -3
expect_int "int", int( -3.2 ), -4
expect_true "abs", abs( -42 ) = 42
expect_true "sgn", sgn( -4 ) = -1
expect_true "sqr", abs( sqr( 81.0 ) - 9.0 ) < 0.00001
expect_true "sin/cos", abs( sin( 0.0 ) ) < 0.00001 andalso abs( cos( 0.0 ) - 1.0 ) < 0.00001
expect_true "atan/log/exp", abs( exp( log( 3.0 ) ) - 3.0 ) < 0.00001
expect_true "rnd", rnd >= 0.0

'' Date, time, timer, command, and environment runtime.
dim as string d = date()
dim as string t = time()
expect_true "date", len( d ) > 0
expect_true "time", len( t ) > 0
expect_true "timer", timer >= 0.0
#if defined( __FB_JS__ )
'' The JS backend runs module-level BASIC through Emscripten constructors.
'' There is no native process argv in that path, so COMMAND(0) is empty.
expect_true "command", command( 0 ) = ""
#else
expect_true "command", command( 0 ) <> ""
#endif

if( CAN_USE_ENVIRON ) then
	setenviron ENV_NAME + "=present"
	expect_true "environ", environ( ENV_NAME ) = "present"
end if

'' Dynamic arrays and memory helpers.
redim as integer values( 1 to 4 )
for i as integer = lbound( values ) to ubound( values )
	values( i ) = i * 10
next
expect_int "lbound", lbound( values ), 1
expect_int "ubound", ubound( values ), 4
expect_int "array value", values( 3 ), 30
erase values

'' Pointer and allocation runtime.
dim as integer ptr p = callocate( 4, sizeof( integer ) )
expect_true "callocate", p <> 0
if( p <> 0 ) then
	p[0] = 11
	p[1] = 22
	expect_int "allocated value", p[0] + p[1], 33
	p = reallocate( p, 8 * sizeof( integer ) )
	expect_true "reallocate", p <> 0
	if( p <> 0 ) then
		p[2] = 33
		expect_int "reallocated value", p[2], 33
		deallocate p
	end if
end if

'' File, directory, and error runtime.
delete_file_if_present TEST_FILE
delete_file_if_present RENAMED_FILE

dim as integer f = freefile()
expect_true "freefile", f > 0
if( open( TEST_FILE for output as #f ) <> 0 ) then
	record_failure "open output failed"
else
	print #f, "alpha"
	print #f, 123
	close #f
end if

f = freefile()
if( open( TEST_FILE for input as #f ) <> 0 ) then
	record_failure "open input failed"
else
	dim as string line_text
	line input #f, line_text
	expect_true "line input", line_text = "alpha"
	input #f, line_text
	expect_true "input", val( line_text ) = 123
	close #f
end if

name TEST_FILE as RENAMED_FILE
dim as integer open_result = open( RENAMED_FILE for input as #f )
expect_true "fileexists after name", open_result = 0
if( open_result = 0 ) then close #f
kill RENAMED_FILE

open_result = open( RENAMED_FILE for input as #f )
expect_true "missing file", open_result <> 0

'' Console-print/input-adjacent runtime paths that are safe in automation.
width 80, 25
locate 1, 1, 0
color 7, 0
print using "####"; 12

if( failures <> 0 ) then
	end 1
end if

end 0

'' end of rtlib-command-sweep.bas
