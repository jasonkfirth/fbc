#include "fbgfx.bi"
using fb

function get_ascii(a as integer) as string
	if (a <> 0) then
		return "'" & chr(a) & "'"
	else
		return "unknown key"
	end if
end function

function get_button(b as integer) as string
	dim result as string = "middle"

	if (b = BUTTON_LEFT) then
		result = "left"
	elseif (b = BUTTON_RIGHT) then
		result = "right"
	end if

	return result
end function

const SCREEN_WIDTH = 640
const SCREEN_HEIGHT = 480
const SCREEN_DEPTH = 32

screenres SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DEPTH

if screenptr = 0 then
	print "Unable to create the graphics window."
	end 1
end if

dim e as EVENT
do
	while screenevent( @e ) <> 0
		select case as const e.type
		case EVENT_KEY_PRESS
			if e.scancode = SC_ESCAPE then
				exit do
			end if
			print get_ascii(e.ascii) & " was pressed (scancode " & e.scancode & ")"
		case EVENT_KEY_RELEASE
			print get_ascii(e.ascii) & " was released (scancode " & e.scancode & ")"
		case EVENT_KEY_REPEAT
			print get_ascii(e.ascii) & " is being repeated (scancode " & e.scancode & ")"
		case EVENT_MOUSE_MOVE
			print "mouse moved to " & e.x & "," & e.y & " (delta " & e.dx & "," & e.dy & ")"
		case EVENT_MOUSE_BUTTON_PRESS
			print get_button(e.button) & " mouse button pressed"
		case EVENT_MOUSE_BUTTON_RELEASE
			print get_button(e.button) & " mouse button released"
		case EVENT_MOUSE_DOUBLE_CLICK
			print get_button(e.button) & " mouse button double clicked"
		case EVENT_MOUSE_WHEEL
			print "mouse wheel moved to position " & e.z
		case EVENT_MOUSE_ENTER
			print "mouse moved into program window"
		case EVENT_MOUSE_EXIT
			print "mouse moved out of program window"
		case EVENT_WINDOW_GOT_FOCUS
			print "program window got focus"
		case EVENT_WINDOW_LOST_FOCUS
			print "program window lost focus"
		case EVENT_WINDOW_CLOSE
			exit do
		case fb.EVENT_MOUSE_HWHEEL
			print "horizontal mouse wheel moved to position " & e.w
		end select
	wend

	sleep 50, 1
loop
