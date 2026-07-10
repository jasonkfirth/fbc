''
'' FreeBASIC macOS smoke tests
'' ---------------------------
''
'' File: curses-smoke.bas
''
'' Purpose:
''
''     Verify that the generic curses wrapper selects the ncurses library
''     shipped with macOS instead of requiring PDCurses.
''
'' Responsibilities:
''
''     - include the generic curses.bi entry point
''     - verify Darwin scalar and opaque-handle layouts at compile time
''     - initialize ncurses against temporary files instead of a real terminal
''     - round-trip a chtype array and use exported WINDOW accessors
''
'' This file intentionally does NOT contain:
''
''     - interactive terminal input
''     - output to the user's terminal
''     - assumptions about optional terminal types
''

#include once "curses.bi"

const SMOKE_OK = 0
const SMOKE_VERSION_FAILED = 1
const SMOKE_TEMPFILE_FAILED = 2
const SMOKE_NEWTERM_FAILED = 3
const SMOKE_NEWWIN_FAILED = 4
const SMOKE_CHARACTER_WRITE_FAILED = 5
const SMOKE_CHARACTER_READ_FAILED = 6
const SMOKE_CHARACTER_DATA_FAILED = 7
const SMOKE_GEOMETRY_FAILED = 8
const SMOKE_ATTRIBUTE_FAILED = 9
const SMOKE_ACCESSOR_FAILED = 10
const SMOKE_CLEANUP_FAILED = 11

#assert sizeof( chtype ) = 4
#assert sizeof( attr_t ) = 4
#assert sizeof( mmask_t ) = 8
#assert sizeof( NCURSES_ATTR_T ) = 4
#assert sizeof( MEVENT ) = 24
#assert sizeof( WINDOW_ ) = 0
#assert sizeof( SCREEN_ ) = 0
#assert sizeof( A_ATTRIBUTES ) = 4

dim version as const zstring ptr = curses_version()
if( version = 0 ) then
	end SMOKE_VERSION_FAILED
end if

if( len(*version) = 0 ) then
	end SMOKE_VERSION_FAILED
end if

dim as integer smoke_result = SMOKE_OK
dim as FILE ptr input_file = tmpfile()
dim as FILE ptr output_file = tmpfile()
dim as SCREEN_ ptr current_screen
dim as WINDOW_ ptr current_window
dim as chtype source_cells(0 to 2) = _
	{ _
		cast( chtype, asc( "A" ) ), _
		cast( chtype, asc( "B" ) ), _
		0 _
	}
dim as chtype result_cells(0 to 2)
dim as attr_t current_attributes
dim as short current_color_pair
dim as long top_row
dim as long bottom_row

if( input_file = 0 orelse output_file = 0 ) then
	smoke_result = SMOKE_TEMPFILE_FAILED
	goto cleanup
end if

'' "dumb" is the minimal terminal description shipped with macOS.  Directing
'' both streams to temporary files keeps this test noninteractive and prevents
'' ncurses control sequences from reaching the user's terminal.
current_screen = newterm( @"dumb", output_file, input_file )
if( current_screen = 0 ) then
	smoke_result = SMOKE_NEWTERM_FAILED
	goto cleanup
end if

set_term current_screen
current_window = newwin( 4, 12, 0, 0 )
if( current_window = 0 ) then
	smoke_result = SMOKE_NEWWIN_FAILED
	goto cleanup
end if

if( waddchnstr( current_window, @source_cells(0), 2 ) = ERR_ ) then
	smoke_result = SMOKE_CHARACTER_WRITE_FAILED
	goto cleanup
end if

if( wmove( current_window, 0, 0 ) = ERR_ ) then
	smoke_result = SMOKE_CHARACTER_READ_FAILED
	goto cleanup
end if

if( winchnstr( current_window, @result_cells(0), 2 ) <> 2 ) then
	smoke_result = SMOKE_CHARACTER_READ_FAILED
	goto cleanup
end if

if( (result_cells(0) and A_CHARTEXT) <> asc( "A" ) orelse _
    (result_cells(1) and A_CHARTEXT) <> asc( "B" ) ) then
	smoke_result = SMOKE_CHARACTER_DATA_FAILED
	goto cleanup
end if

if( getcurx( current_window ) <> 0 orelse _
    getcury( current_window ) <> 0 orelse _
    getmaxx( current_window ) <> 12 orelse _
    getmaxy( current_window ) <> 4 ) then
	smoke_result = SMOKE_GEOMETRY_FAILED
	goto cleanup
end if

if( wattr_set( current_window, A_BOLD, 0, 0 ) = ERR_ ) then
	smoke_result = SMOKE_ATTRIBUTE_FAILED
	goto cleanup
end if

if( wattr_get( current_window, @current_attributes, @current_color_pair, 0 ) = ERR_ orelse _
    (current_attributes and A_BOLD) = 0 ) then
	smoke_result = SMOKE_ATTRIBUTE_FAILED
	goto cleanup
end if

if( keypad( current_window, CTRUE ) = ERR_ orelse _
    is_keypad( current_window ) = FALSE orelse _
    wgetparent( current_window ) <> 0 ) then
	smoke_result = SMOKE_ACCESSOR_FAILED
	goto cleanup
end if

if( wsetscrreg( current_window, 1, 2 ) = ERR_ ) then
	smoke_result = SMOKE_ACCESSOR_FAILED
	goto cleanup
end if

if( wgetscrreg( current_window, @top_row, @bottom_row ) = ERR_ orelse _
    top_row <> 1 orelse bottom_row <> 2 ) then
	smoke_result = SMOKE_ACCESSOR_FAILED
end if

cleanup:

if( current_window <> 0 ) then
	if( delwin( current_window ) = ERR_ andalso smoke_result = SMOKE_OK ) then
		smoke_result = SMOKE_CLEANUP_FAILED
	end if
end if

if( current_screen <> 0 ) then
	endwin()
	delscreen current_screen
end if

if( input_file <> 0 ) then
	fclose input_file
end if

if( output_file <> 0 ) then
	fclose output_file
end if

end smoke_result

'' end of curses-smoke.bas
