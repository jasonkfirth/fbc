''
'' Project: FreeBASIC examples
'' ---------------------------
''
'' File: active-state.bas
''
'' Purpose:
''
''     Show which GETXPAD devices are active and display their current state.
''
'' Responsibilities:
''
''     - scan the example GETXPAD pad range
''     - show connected, disconnected, and missing slots
''     - display current buttons, sticks, triggers, and d-pad state
''
'' This file intentionally does NOT contain:
''
''     - controller calibration
''     - per-game control bindings
''     - force feedback or rumble support
''

#include "xpad_common.bi"

dim pads( 0 to XPAD_EXAMPLE_MAX_PADS - 1 ) as XPAD_EXAMPLE_STATE
dim as integer active_count
dim as integer disconnected_count
dim as integer done
dim as integer i

if screenres( 800, 600, 32 ) <> 0 then
	print "Unable to open a graphics screen."
	end 1
end if

width 100, 37

do
	active_count = 0
	disconnected_count = 0
	done = multikey( SC_ESCAPE )

	for i = 0 to XPAD_EXAMPLE_MAX_PADS - 1
		xpad_poll i, pads( i )

		select case as const pads( i ).status
		case XPAD_STATUS_CONNECTED
			active_count += 1
			if xpad_exit_pressed( pads( i ) ) then
				done = -1
			end if
		case XPAD_STATUS_DISCONNECTED
			disconnected_count += 1
		end select
	next

	cls
	color rgb( 255, 255, 255 ), rgb( 0, 0, 0 )

	locate 2, 3
	print "GETXPAD active controller monitor"
	locate 3, 3
	print "ESC or Start+Select on any connected controller exits."
	locate 5, 3
	print "Active xpads:"; active_count; "   disconnected slots:"; disconnected_count

	locate 7, 3
	print "Pad  Status        Buttons                        D-pad       Sticks and triggers"

	for i = 0 to XPAD_EXAMPLE_MAX_PADS - 1
		select case as const pads( i ).status
		case XPAD_STATUS_CONNECTED
			color rgb( 120, 255, 150 ), rgb( 0, 0, 0 )
		case XPAD_STATUS_DISCONNECTED
			color rgb( 255, 220, 90 ), rgb( 0, 0, 0 )
		case else
			color rgb( 120, 120, 120 ), rgb( 0, 0, 0 )
		end select

		locate 9 + i, 3
		print xpad_fit( ltrim( str( i ) ), 2 ); "   ";
		print xpad_fit( xpad_status_name( pads( i ).status ), 12 ); "  ";

		if pads( i ).status = XPAD_STATUS_CONNECTED then
			print xpad_fit( xpad_button_list( pads( i ).buttons ), 30 ); " ";
			print xpad_fit( xpad_dpad_name( pads( i ).dpad ), 10 ); " ";
			print "LS "; xpad_axis_text( pads( i ).lstick_x ); ","; xpad_axis_text( pads( i ).lstick_y ); "  ";
			print "RS "; xpad_axis_text( pads( i ).rstick_x ); ","; xpad_axis_text( pads( i ).rstick_y ); "  ";
			print "LT "; xpad_trigger_text( pads( i ).ltrigger ); "  ";
			print "RT "; xpad_trigger_text( pads( i ).rtrigger );
		else
			print "no current controller state";
		end if
	next

	color rgb( 160, 160, 160 ), rgb( 0, 0, 0 )
	locate 27, 3
	print "Windows and Xbox normally expose four pads. Linux joystick devices may expose more."
	locate 28, 3
	print "A disconnected slot means GETXPAD saw that pad earlier in this process."

	sleep 25, 1
loop until done

'' end of active-state.bas
