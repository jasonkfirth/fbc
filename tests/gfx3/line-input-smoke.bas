''
'' Project: FreeBASIC gfxlib3 tests
'' --------------------------------
''
'' File: line-input-smoke.bas
''
'' Purpose:
''
''     Verify that graphical LINE INPUT uses the gfxlib3 console and native
''     keyboard queues.
''
'' Responsibilities:
''
''     - inject deterministic character and editing-key messages
''     - check narrow LINE INPUT insertion and cursor movement
''     - check wide LINE INPUT byte-to-wide conversion
''     - check the INPUT$ standard-input byte-reader hook
''     - verify edited text remains visible in graphical console cells
''
'' This file intentionally does NOT contain:
''
''     - physical keyboard interaction
''     - non-Win32 platform input checks
''     - Unicode input-method or composition testing
''

#define __FB_GFXLIB3__
#include once "fbgfx.bi"
#include once "windows.bi"

const native_title = "gfxlib3 line input smoke"

extern "C"
	declare function __acrt_iob_func( byval index as uinteger ) as any ptr
	declare function fb_ReadString alias "fb_ReadString"( _
		byval buffer as zstring ptr, byval maximum_length as longint, _
		byval stream as any ptr ) as zstring ptr
end extern

sub post_character( byval native_window as HWND, byval character as integer )
	PostMessage( native_window, WM_CHAR, character, 1 )
end sub

if screenres( 160, 64, 32, 1, 0 ) <> 0 then end 1
windowtitle native_title
screensync

dim as HWND native_window = FindWindow( 0, strptr( native_title ) )
if native_window = 0 then end 2
PostMessage( native_window, WM_ACTIVATE, makelong( WA_ACTIVE, 0 ), 0 )

'' Enter "ac", move left, insert "b", then accept the edited line.
post_character native_window, asc( "a" )
post_character native_window, asc( "c" )
dim as LPARAM left_key_data = 1 or (MapVirtualKey( VK_LEFT, 0 ) shl 16)
PostMessage( native_window, WM_KEYDOWN, VK_LEFT, left_key_data )
post_character native_window, asc( "b" )
post_character native_window, 13

dim as string narrow_result
line input ; narrow_result
if narrow_result <> "abc" then end 3
if screen( 1, 1 ) <> asc( "a" ) then end 4
if screen( 1, 2 ) <> asc( "b" ) then end 5
if screen( 1, 3 ) <> asc( "c" ) then end 6

locate 2, 1
post_character native_window, asc( "w" )
post_character native_window, asc( "i" )
post_character native_window, asc( "d" )
post_character native_window, asc( "e" )
post_character native_window, 13

dim as wstring * 16 wide_result
line input ; wide_result
if wide_result <> wstr( "wide" ) then end 7
if screen( 2, 1 ) <> asc( "w" ) then end 8
if screen( 2, 4 ) <> asc( "e" ) then end 9

locate 3, 1
post_character native_window, asc( "x" )
post_character native_window, asc( "y" )
post_character native_window, asc( "z" )
post_character native_window, 13
dim as zstring * 16 byte_result
if fb_ReadString( @byte_result, 16, __acrt_iob_func( 0 ) ) = 0 then end 10
if left( byte_result, 3 ) <> "xyz" then end 10
if byte_result[3] <> 13 then end 10
if screen( 3, 1 ) <> asc( "x" ) then end 11
if screen( 3, 3 ) <> asc( "z" ) then end 12

screen 0
end 0

'' end of line-input-smoke.bas
